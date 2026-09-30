// Dataset readers on small generated fixtures in the TUM RGB-D, EuRoC and
// BaekAR layouts, plus format detection and the engine-size decorator.

#include "adapters/frame_source/DatasetSources.h"
#include "adapters/frame_source/FrameSources.h"
#include "core/Rotation.h"

#include <opencv2/imgcodecs.hpp>

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace baekar;

namespace {

int failures = 0;

void expect(bool condition, const char* what) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", what);
        ++failures;
    }
}

bool near(double a, double b, double tolerance = 1e-6) { return std::abs(a - b) <= tolerance; }

// Deterministic per seed, so a test can rebuild the image it wrote.
cv::Mat pattern(int width, int height, int seed, int type = CV_8UC3) {
    cv::Mat image(height, width, type);
    cv::RNG rng(static_cast<std::uint64_t>(seed) + 1);
    rng.fill(image, cv::RNG::UNIFORM, cv::Scalar::all(0), cv::Scalar::all(256));
    return image;
}

void writeTum(const fs::path& root) {
    fs::create_directories(root / "rgb");
    fs::create_directories(root / "depth");
    std::ofstream rgb(root / "rgb.txt"), depth(root / "depth.txt"), truth(root / "groundtruth.txt");
    rgb << "# color images\n# file: 'fixture'\n# timestamp filename\n";
    depth << "# depth maps\n# file: 'fixture'\n# timestamp filename\n";
    truth << "# ground truth trajectory\n# timestamp tx ty tz qx qy qz qw\n";
    for (int i = 0; i < 5; ++i) {
        const double t = 1305031102.175304 + i * 0.033;
        char name[64];
        std::snprintf(name, sizeof(name), "%.6f.png", t);
        cv::imwrite((root / "rgb" / name).string(), pattern(640, 480, i));
        cv::Mat d(480, 640, CV_16UC1, cv::Scalar(5000 * (i + 1)));  // (i+1) metres
        std::snprintf(name, sizeof(name), "%.6f.png", t + 0.005);
        cv::imwrite((root / "depth" / name).string(), d);
        std::snprintf(name, sizeof(name), "%.6f", t);
        rgb << name << " rgb/" << name << ".png\n";
        std::snprintf(name, sizeof(name), "%.6f", t + 0.005);
        depth << name << " depth/" << name << ".png\n";
        truth << std::fixed << t + 0.002 << ' ' << i * 0.1 << " 0.2 0.3 0 0 0 1\n";
    }
}

void writeEuroc(const fs::path& root) {
    const fs::path mav = root / "mav0";
    fs::create_directories(mav / "cam0" / "data");
    fs::create_directories(mav / "imu0");
    fs::create_directories(mav / "state_groundtruth_estimate0");
    std::ofstream yaml(mav / "cam0" / "sensor.yaml");
    yaml << "# General sensor definitions.\nsensor_type: camera\n"
            "T_BS:\n  cols: 4\n  rows: 4\n  data: [0.0, -1.0, 0.0, 0.1,\n"
            "         1.0, 0.0, 0.0, 0.2,\n         0.0, 0.0, 1.0, 0.3,\n         0.0, 0.0, 0.0, 1.0]\n"
            "rate_hz: 20\nresolution: [752, 480]\ncamera_model: pinhole\n"
            "intrinsics: [458.654, 457.296, 367.215, 248.375] #fu, fv, cu, cv\n"
            "distortion_model: radial-tangential\n"
            "distortion_coefficients: [-0.28340811, 0.07395907, 0.00019359, 1.76187114e-05]\n";
    std::ofstream cams(mav / "cam0" / "data.csv"), imu(mav / "imu0" / "data.csv"),
        truth(mav / "state_groundtruth_estimate0" / "data.csv");
    cams << "#timestamp [ns],filename\n";
    imu << "#timestamp [ns],w_RS_S_x [rad s^-1],w_RS_S_y,w_RS_S_z,a_RS_S_x [m s^-2],a_RS_S_y,a_RS_S_z\n";
    truth << "#timestamp, p_RS_R_x [m], p_RS_R_y [m], p_RS_R_z [m], q_RS_w [], q_RS_x [], q_RS_y [], q_RS_z []\n";
    const long long start = 1403636579763555584LL;
    for (int i = 0; i < 4; ++i) {
        const long long t = start + i * 50000000LL;  // 20 Hz
        const std::string name = std::to_string(t) + ".png";
        cv::imwrite((mav / "cam0" / "data" / name).string(), pattern(752, 480, i, CV_8UC1));
        cams << t << ',' << name << '\n';
        for (int k = 0; k < 10; ++k)  // 200 Hz
            imu << t - 50000000LL + (k + 1) * 5000000LL << ",0.01,0.02,0.03,9.8,0.1,0.2\n";
        truth << t << ',' << 1.0 + i << ",2.0,3.0,1.0,0.0,0.0,0.0\n";  // identity rotation
    }
}

}  // namespace

int main() {
    const fs::path base = fs::temp_directory_path() / "baekar_dataset_test";
    fs::remove_all(base);

    // ---- TUM RGB-D
    const fs::path tumDir = base / "rgbd_dataset_freiburg1_fixture";
    writeTum(tumDir);
    {
        auto source = openDatasetDirectory(tumDir.string(), false);
        expect(source && dynamic_cast<TumRgbdFrameSource*>(source.get()), "TUM layout detected");
        expect(source && source->open(), "TUM opens");
        Frame f;
        int count = 0;
        while (source && source->read(f)) {
            ++count;
            if (count == 3) {
                expect(f.intrinsics && near(f.intrinsics->fx, 517.3) && near(f.intrinsics->distortion[0], 0.2624),
                       "freiburg1 intrinsics");
                expect(!f.depth.empty() && near(f.depth.at<float>(10, 10), 3.0f, 1e-4), "depth in metres (/5000)");
                expect(f.referencePose && near(f.referencePose->t[0], 0.2), "ground truth associated");
                expect(f.bgr.cols == 640 && f.bgr.rows == 480 && f.bgr.channels() == 3, "colour image");
            }
        }
        expect(count == 5, "TUM frame count");
    }

    // ---- EuRoC
    const fs::path eurocDir = base / "MH_fixture";
    writeEuroc(eurocDir);
    {
        auto source = openDatasetDirectory(eurocDir.string(), false);
        expect(source && dynamic_cast<EurocFrameSource*>(source.get()), "EuRoC layout detected");
        expect(source && source->open(), "EuRoC opens");
        Frame f;
        int count = 0;
        std::size_t imu = 0;
        while (source && source->read(f)) {
            ++count;
            imu += f.imu.size();
            if (count == 2) {
                expect(f.bgr.cols == 752 && f.bgr.channels() == 3, "greyscale loaded as BGR");
                expect(f.intrinsics && near(f.intrinsics->fx, 458.654) && f.intrinsics->width == 752,
                       "EuRoC intrinsics from sensor.yaml");
                // world_from_camera = world_from_body * body_from_camera.
                expect(f.referencePose && near(f.referencePose->t[0], 2.1) && near(f.referencePose->t[1], 2.2) &&
                           near(f.referencePose->t[2], 3.3),
                       "camera pose uses T_BS");
                expect(f.referencePose && near(f.referencePose->R(0, 1), -1.0), "camera rotation uses T_BS");
                expect(f.imu.size() == 10, "IMU samples since the previous frame");
            }
        }
        expect(count == 4 && imu == 40, "EuRoC frames and IMU");

        EngineSizeFrameSource sized(openDatasetDirectory(eurocDir.string(), false));
        expect(sized.open() && sized.read(f), "engine-size decorator reads");
        expect(f.bgr.cols == 640 && f.bgr.rows == 480, "resized to 640x480");
        expect(f.intrinsics && near(f.intrinsics->fx, 458.654 * 640.0 / 752.0, 1e-6) && f.intrinsics->width == 640,
               "intrinsics scaled with the image");
    }

    // ---- BaekAR format round trip
    const fs::path baekarDir = base / "baekar";
    {
        BaekarDatasetWriter writer;
        expect(writer.open(baekarDir.string(), "fixture device"), "writer opens");
        for (int i = 0; i < 3; ++i) {
            Frame f;
            f.bgr = pattern(320, 240, i);
            f.sequence = i + 1;
            f.timestampSeconds = 10.0 + i * 0.1;
            f.depth = cv::Mat(240, 320, CV_32F, cv::Scalar(0.5 * (i + 1)));
            CameraIntrinsics k;
            k.fx = 300; k.fy = 301; k.cx = 160; k.cy = 120; k.width = 320; k.height = 240;
            k.distortion = {0.1, -0.2, 0, 0, 0};
            f.intrinsics = k;
            RigidTransform pose;
            pose.t = cv::Vec3d(i, 2 * i, 3);
            pose.R = quaternionToRotation(0, 0, std::sin(0.1 * i), std::cos(0.1 * i));
            f.referencePose = pose;
            f.imu.push_back({f.timestampSeconds - 0.05, cv::Vec3d(0.1, 0.2, 0.3), cv::Vec3d(0, 9.8, 0)});
            expect(writer.write(f), "writer writes");
        }
        expect(writer.close() && writer.framesWritten() == 3, "writer closes");

        auto source = openDatasetDirectory(baekarDir.string(), false);
        expect(source && dynamic_cast<BaekarDatasetFrameSource*>(source.get()), "BaekAR layout detected");
        expect(source && source->open(), "BaekAR opens");
        Frame f;
        int count = 0;
        while (source && source->read(f)) {
            const cv::Mat expected = pattern(320, 240, count);
            expect(cv::norm(f.bgr, expected, cv::NORM_INF) == 0.0, "pixels round trip");
            expect(near(f.timestampSeconds, 10.0 + count * 0.1), "timestamp round trip");
            expect(!f.depth.empty() && near(f.depth.at<float>(5, 5), 0.5 * (count + 1), 1e-3), "depth round trip");
            expect(f.intrinsics && near(f.intrinsics->fy, 301) && near(f.intrinsics->distortion[1], -0.2),
                   "intrinsics round trip");
            expect(f.referencePose && near(f.referencePose->t[1], 2.0 * count, 1e-6), "pose round trip");
            expect(f.imu.size() == 1 && near(f.imu[0].gyro[2], 0.3), "IMU round trip");
            ++count;
        }
        expect(count == 3, "BaekAR frame count");
    }

    // ---- plain image folder
    const fs::path plainDir = base / "plain";
    fs::create_directories(plainDir);
    cv::imwrite((plainDir / "a.png").string(), pattern(640, 480, 1));
    expect(dynamic_cast<ImageSequenceFrameSource*>(openDatasetDirectory(plainDir.string(), false).get()) != nullptr,
           "plain image folder detected");

    fs::remove_all(base);
    if (failures == 0) std::fprintf(stderr, "dataset_sources_test: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
