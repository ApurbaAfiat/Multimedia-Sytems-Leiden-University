#include <darknet.hpp>
#include <opencv2/opencv.hpp>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

constexpr double kIouThreshold = 0.50;
constexpr float kDetectionThreshold = 0.24f;

struct GroundTruth {
    int class_id = -1;
    cv::Rect2d rect;
};

struct MatchCandidate {
    std::size_t gt_index = 0;
    std::size_t pred_index = 0;
    double iou = 0.0;
};

struct Stats {
    int images = 0;
    int tp = 0;
    int fp = 0;
    int fn = 0;
    double iou_sum = 0.0;
    double inference_ms_sum = 0.0;
};

// -----------------------------------------------------------------------------
// STUDENT TASK
// Implement Intersection over Union (IoU) for two rectangles.
// Return a value in [0, 1].
// -----------------------------------------------------------------------------
double calculate_iou(const cv::Rect2d &a, const cv::Rect2d &b)
{
    // Intersection rectangle: the overlap region of a and b.
    const double inter_x1 = std::max(a.x, b.x);
    const double inter_y1 = std::max(a.y, b.y);
    const double inter_x2 = std::min(a.x + a.width,  b.x + b.width);
    const double inter_y2 = std::min(a.y + a.height, b.y + b.height);

    // If the rectangles do not overlap, width/height would be negative; clamp to 0.
    const double inter_w = std::max(0.0, inter_x2 - inter_x1);
    const double inter_h = std::max(0.0, inter_y2 - inter_y1);
    const double inter_area = inter_w * inter_h;

    // Union = area_a + area_b - intersection  (avoids double-counting the overlap).
    const double area_a = a.width * a.height;
    const double area_b = b.width * b.height;
    const double union_area = area_a + area_b - inter_area;

    // Guard against degenerate zero-area boxes.
    if (union_area <= 0.0) {
        return 0.0;
    }

    return inter_area / union_area;
}

std::vector<GroundTruth> load_ground_truth(const fs::path &filename)
{
    std::ifstream in(filename);
    if (!in) {
        throw std::runtime_error("Cannot open ground-truth file: " + filename.string());
    }

    std::vector<GroundTruth> result;
    GroundTruth gt;
    while (in >> gt.class_id >> gt.rect.x >> gt.rect.y >> gt.rect.width >> gt.rect.height) {
        result.push_back(gt);
    }
    return result;
}

double prediction_confidence(const Darknet::Prediction &prediction)
{
    const auto it = prediction.prob.find(prediction.best_class);
    if (it != prediction.prob.end()) {
        return it->second;
    }
    double best = 0.0;
    for (const auto &[class_id, prob] : prediction.prob) {
        (void)class_id;
        best = std::max(best, static_cast<double>(prob));
    }
    return best;
}

void add_stats(Stats &dst, const Stats &src)
{
    dst.images += src.images;
    dst.tp += src.tp;
    dst.fp += src.fp;
    dst.fn += src.fn;
    dst.iou_sum += src.iou_sum;
    dst.inference_ms_sum += src.inference_ms_sum;
}

double precision(const Stats &s)
{
    const int denom = s.tp + s.fp;
    return denom > 0 ? static_cast<double>(s.tp) / denom : 0.0;
}

double recall(const Stats &s)
{
    const int denom = s.tp + s.fn;
    return denom > 0 ? static_cast<double>(s.tp) / denom : 0.0;
}

double mean_iou(const Stats &s)
{
    return s.tp > 0 ? s.iou_sum / s.tp : 0.0;
}

double average_inference_ms(const Stats &s)
{
    return s.images > 0 ? s.inference_ms_sum / s.images : 0.0;
}

void print_summary(const std::string &title, const Stats &s)
{
    std::cout << "\n=== " << title << " ===\n"
              << "Images: " << s.images << '\n'
              << "TP: " << s.tp << '\n'
              << "FP: " << s.fp << '\n'
              << "FN: " << s.fn << '\n'
              << "Precision: " << precision(s) << '\n'
              << "Recall: " << recall(s) << '\n'
              << "Mean IoU: " << mean_iou(s) << '\n'
              << "Average inference: " << average_inference_ms(s) << " ms\n";
}

bool run_iou_self_test()
{
    const cv::Rect2d a(0.0, 0.0, 100.0, 100.0);
    const cv::Rect2d b(50.0, 50.0, 100.0, 100.0);
    const double expected = 2500.0 / 17500.0; // 1/7
    const double observed = calculate_iou(a, b);
    const bool ok = std::abs(observed - expected) < 1e-6;

    std::cout << std::fixed << std::setprecision(6)
              << "IoU self-test\n"
              << "Expected: " << expected << '\n'
              << "Observed: " << observed << '\n'
              << (ok ? "PASS" : "FAIL") << '\n';
    return ok;
}

int main(int argc, char **argv)
{
    try {
        if (argc == 2 && std::string(argv[1]) == "--test-iou") {
            return run_iou_self_test() ? 0 : 1;
        }

        if (argc != 6) {
            std::cerr
                << "Usage:\n  " << argv[0]
                << " model.cfg classes.names model.weights dataset_dir selected_images.txt\n\n"
                << "IoU test:\n  " << argv[0] << " --test-iou\n";
            return 1;
        }

        const fs::path cfg = argv[1];
        const fs::path names = argv[2];
        const fs::path weights = argv[3];
        const fs::path dataset_dir = argv[4];
        const fs::path manifest = argv[5];

        Darknet::NetworkPtr net = Darknet::load_neural_network(cfg, names, weights);
        if (!net) {
            throw std::runtime_error("Failed to load Darknet network.");
        }
        Darknet::set_detection_threshold(net, kDetectionThreshold);
        const auto &class_names = Darknet::get_class_names(net);

        const std::string model_name = cfg.stem().string();
        const fs::path output_dir = fs::path("output") / model_name;
        fs::create_directories(output_dir);

        std::ifstream list(manifest);
        if (!list) {
            throw std::runtime_error("Cannot open manifest: " + manifest.string());
        }

        std::map<std::string, Stats> bucket_stats;
        Stats overall;

        std::string image_name;
        std::string bucket;
        std::cout << std::fixed << std::setprecision(6);

        while (list >> image_name >> bucket) {
            const fs::path image_path = dataset_dir / "images" / image_name;
            const fs::path gt_path = dataset_dir / "ground_truth" /
                                     (fs::path(image_name).stem().string() + ".txt");

            cv::Mat image = cv::imread(image_path.string());
            if (image.empty()) {
                throw std::runtime_error("Cannot read image: " + image_path.string());
            }

            const auto gt = load_ground_truth(gt_path);

            const auto start = std::chrono::steady_clock::now();
            const auto predictions = Darknet::predict(net, image);
            const auto stop = std::chrono::steady_clock::now();
            const double inference_ms =
                std::chrono::duration<double, std::milli>(stop - start).count();

            std::vector<MatchCandidate> candidates;
            for (std::size_t gi = 0; gi < gt.size(); ++gi) {
                for (std::size_t pi = 0; pi < predictions.size(); ++pi) {
                    if (predictions[pi].best_class != gt[gi].class_id) {
                        continue;
                    }
                    const cv::Rect2d pred_rect(predictions[pi].rect);
                    const double iou = calculate_iou(gt[gi].rect, pred_rect);
                    if (iou >= kIouThreshold) {
                        candidates.push_back({gi, pi, iou});
                    }
                }
            }

            std::sort(candidates.begin(), candidates.end(),
                      [](const MatchCandidate &a, const MatchCandidate &b) {
                          return a.iou > b.iou;
                      });

            std::vector<bool> gt_used(gt.size(), false);
            std::vector<bool> pred_used(predictions.size(), false);
            Stats image_stats;
            image_stats.images = 1;
            image_stats.inference_ms_sum = inference_ms;

            for (const auto &candidate : candidates) {
                if (gt_used[candidate.gt_index] || pred_used[candidate.pred_index]) {
                    continue;
                }
                gt_used[candidate.gt_index] = true;
                pred_used[candidate.pred_index] = true;
                ++image_stats.tp;
                image_stats.iou_sum += candidate.iou;
            }

            image_stats.fn = static_cast<int>(
                std::count(gt_used.begin(), gt_used.end(), false));
            image_stats.fp = static_cast<int>(
                std::count(pred_used.begin(), pred_used.end(), false));

            add_stats(bucket_stats[bucket], image_stats);
            add_stats(overall, image_stats);

            std::cout << image_name << " [" << bucket << "]"
                      << " GT=" << gt.size()
                      << " Pred=" << predictions.size()
                      << " TP=" << image_stats.tp
                      << " FP=" << image_stats.fp
                      << " FN=" << image_stats.fn
                      << " Precision=" << precision(image_stats)
                      << " Recall=" << recall(image_stats)
                      << " MeanIoU=" << mean_iou(image_stats)
                      << " Time=" << inference_ms << " ms\n";

            for (const auto &prediction : predictions) {
                const int cls = prediction.best_class;
                const std::string class_name =
                    (cls >= 0 && static_cast<std::size_t>(cls) < class_names.size())
                    ? class_names[cls]
                    : std::to_string(cls);
                const auto &r = prediction.rect;
                std::cout << "  -> " << class_name
                          << " conf=" << prediction_confidence(prediction)
                          << " bbox=" << r.x << ',' << r.y << ',' << r.width << ',' << r.height
                          << '\n';
            }

            cv::Mat annotated = Darknet::annotate(net, predictions, image.clone());
            cv::imwrite((output_dir / image_name).string(), annotated);
        }

        for (const std::string bucket_name : {"easy", "medium", "difficult"}) {
            print_summary(bucket_name, bucket_stats[bucket_name]);
        }
        print_summary("OVERALL", overall);

        std::cout << "\nAnnotated images saved in: " << output_dir << '\n';
        Darknet::free_neural_network(net);
        return 0;
    }
    catch (const std::exception &e) {
        std::cerr << "ERROR: " << e.what() << '\n';
        return 1;
    }
}
