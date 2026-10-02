#include "adapters/frame_source/DatasetSources.h"

#include "adapters/frame_source/FrameSources.h"
#include "core/Rotation.h"

#include <opencv2/core/persistence.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

namespace baekar {
namespace {

// Splits a CSV or whitespace-separated line; keeps empty fields for CSV.
std::vector<std::string> splitFields(const std::string& line, char separator) {
    std::vector<std::string> fields;
    std::string field;
    std::istringstream in(line);
    if (separator == ' ') {
        while (in >> field) fields.push_back(field);
        return fields;
    }
    while (std::getline(in, field, separator)) {
        field.erase(0, field.find_first_not_of(" \t\r"));
        field.erase(field.find_last_not_of(" \t\r") + 1);
        fields.push_back(field);
    }
    if (!line.empty() && line.back() == separator) fields.emplace_back();
    return fields;
}

// A pose with a timestamp, for associating ground truth with images.
struct StampedPose {
    double timestampSeconds = 0.0;
    RigidTransform pose;
};

bool isComment(const std::string& line) {
    const auto first = line.find_first_not_of(" \t\r");
    return first == std::string::npos || line[first] == '#';
}

// Nearest entry in a time-sorted list, within maxDifference seconds.
template <typename T, typename Time>
const T* nearestInTime(const std::vector<T>& items, double t, double maxDifference, Time time) {
    auto it = std::lower_bound(items.begin(), items.end(), t,
                               [&](const T& item, double value) { return time(item) < value; });
    const T* best = nullptr;
    double bestDifference = maxDifference;
    for (auto candidate : {it, it == items.begin() ? it : std::prev(it)}) {
        if (candidate == items.end()) continue;
        const double d = std::abs(time(*candidate) - t);
        if (d <= bestDifference) {
            bestDifference = d;
            best = &*candidate;
        }
    }
    return best;
}

std::string joinPath(const std::string& directory, const std::string& relative) {
    return (fs::path(directory) / relative).string();
}

// Numbers after `key:` in a YAML-ish file, e.g. "intrinsics: [1, 2, 3]" or a
// block "T_BS:\n  ...\n  data: [ ... ]" (EuRoC sensor.yaml).
std::vector<double> yamlNumbers(const std::string& text, const std::string& key) {
    std::vector<double> numbers;
    auto position = text.find(key + ":");
    if (position == std::string::npos) return numbers;
    auto open = text.find('[', position);
    if (open == std::string::npos) return numbers;
    auto close = text.find(']', open);
    if (close == std::string::npos) return numbers;
    std::string list = text.substr(open + 1, close - open - 1);
    std::replace(list.begin(), list.end(), ',', ' ');
    std::istringstream in(list);
    double value;
    while (in >> value) numbers.push_back(value);
    return numbers;
}

}  // namespace

// ------------------------------------------------------------ shared reader

bool DatasetFrameSource::finishOpen() {
    std::sort(entries_.begin(), entries_.end(),
              [](const DatasetEntry& a, const DatasetEntry& b) { return a.timestampSeconds < b.timestampSeconds; });
    std::sort(imu_.begin(), imu_.end(),
              [](const ImuSample& a, const ImuSample& b) { return a.timestampSeconds < b.timestampSeconds; });
    next_ = 0;
    nextImu_ = 0;
    if (entries_.empty()) {
        std::fprintf(stderr, "Dataset: no frames in %s\n", description_.c_str());
        return false;
    }
    std::fprintf(stderr, "Dataset: %s, %zu frame(s), %zu IMU sample(s)\n", description_.c_str(), entries_.size(),
                 imu_.size());
    return true;
}

bool DatasetFrameSource::read(Frame& frame) {
    if (entries_.empty()) return false;
    if (next_ >= entries_.size()) {
        if (!loop_) return false;
        next_ = 0;
        nextImu_ = 0;
    }
    const DatasetEntry& entry = entries_[next_++];
    cv::Mat image = cv::imread(entry.rgbPath, colourIsGrey_ ? cv::IMREAD_GRAYSCALE : cv::IMREAD_COLOR);
    if (image.empty()) {
        std::fprintf(stderr, "Dataset: cannot read %s\n", entry.rgbPath.c_str());
        return false;
    }
    if (image.channels() == 1) cv::cvtColor(image, image, cv::COLOR_GRAY2BGR);

    Frame next;
    next.bgr = image;
    next.sequence = ++sequence_;
    next.tickCount = cv::getTickCount();
    next.timestampSeconds = entry.timestampSeconds;
    next.intrinsics = intrinsics_;
    if (entry.hasPose) next.referencePose = entry.pose;
    if (!entry.depthPath.empty()) {
        cv::Mat raw = cv::imread(entry.depthPath, cv::IMREAD_ANYDEPTH);
        if (!raw.empty()) raw.convertTo(next.depth, CV_32F, 1.0 / depthUnitsPerMetre_);
    }
    while (nextImu_ < imu_.size() && imu_[nextImu_].timestampSeconds <= entry.timestampSeconds)
        next.imu.push_back(imu_[nextImu_++]);
    frame = std::move(next);
    return true;
}

// ------------------------------------------------------------ BaekAR format

BaekarDatasetFrameSource::BaekarDatasetFrameSource(std::string directory, bool loop)
    : DatasetFrameSource(loop), directory_(std::move(directory)) {
    description_ = "BaekAR dataset " + directory_;
}

bool BaekarDatasetFrameSource::open() {
    entries_.clear();
    imu_.clear();
    cv::FileStorage meta;
    try {
        meta.open(joinPath(directory_, "baekar_dataset.json"), cv::FileStorage::READ | cv::FileStorage::FORMAT_JSON);
    } catch (const cv::Exception&) {
        return false;
    }
    if (!meta.isOpened() || static_cast<std::string>(meta["format"]) != "baekar-dataset") {
        std::fprintf(stderr, "Dataset: %s has no valid baekar_dataset.json\n", directory_.c_str());
        return false;
    }
    const std::string device = static_cast<std::string>(meta["device"]);
    if (!device.empty()) description_ += " (" + device + ")";
    if (!meta["depth_scale"].empty()) depthUnitsPerMetre_ = static_cast<double>(meta["depth_scale"]);
    const cv::FileNode k = meta["intrinsics"];
    if (!k.empty()) {
        CameraIntrinsics intrinsics;
        intrinsics.fx = k["fx"]; intrinsics.fy = k["fy"];
        intrinsics.cx = k["cx"]; intrinsics.cy = k["cy"];
        intrinsics.width = static_cast<int>(k["width"]);
        intrinsics.height = static_cast<int>(k["height"]);
        const cv::FileNode d = k["distortion"];
        for (int i = 0; i < static_cast<int>(d.size()) && i < 5; ++i) intrinsics.distortion[i] = d[i];
        intrinsics_ = intrinsics;
    }

    std::ifstream frames(joinPath(directory_, "frames.csv"));
    std::string line;
    std::getline(frames, line);  // header
    while (std::getline(frames, line)) {
        if (isComment(line)) continue;
        const std::vector<std::string> f = splitFields(line, ',');
        if (f.size() < 2) continue;
        DatasetEntry entry;
        entry.timestampSeconds = std::stod(f[0]);
        entry.rgbPath = joinPath(directory_, f[1]);
        if (f.size() > 2 && !f[2].empty()) entry.depthPath = joinPath(directory_, f[2]);
        if (f.size() >= 10 && !f[3].empty()) {
            entry.hasPose = true;
            entry.pose.t = cv::Vec3d(std::stod(f[3]), std::stod(f[4]), std::stod(f[5]));
            entry.pose.R = quaternionToRotation(std::stod(f[6]), std::stod(f[7]), std::stod(f[8]), std::stod(f[9]));
        }
        entries_.push_back(entry);
    }

    std::ifstream imu(joinPath(directory_, "imu.csv"));
    if (imu) {
        std::getline(imu, line);
        while (std::getline(imu, line)) {
            const std::vector<std::string> f = splitFields(line, ',');
            if (isComment(line) || f.size() < 7) continue;
            ImuSample s;
            s.timestampSeconds = std::stod(f[0]);
            s.gyro = cv::Vec3d(std::stod(f[1]), std::stod(f[2]), std::stod(f[3]));
            s.accel = cv::Vec3d(std::stod(f[4]), std::stod(f[5]), std::stod(f[6]));
            imu_.push_back(s);
        }
    }
    return finishOpen();
}

// ------------------------------------------------------------ TUM RGB-D

TumRgbdFrameSource::TumRgbdFrameSource(std::string directory, bool loop)
    : DatasetFrameSource(loop), directory_(std::move(directory)) {
    description_ = "TUM RGB-D " + directory_;
}

bool TumRgbdFrameSource::open() {
    entries_.clear();
    struct Stamped {
        double t;
        std::string path;
    };
    auto readList = [&](const std::string& name) {
        std::vector<Stamped> list;
        std::ifstream in(joinPath(directory_, name));
        std::string line;
        while (std::getline(in, line)) {
            if (isComment(line)) continue;
            const std::vector<std::string> f = splitFields(line, ' ');
            if (f.size() >= 2) list.push_back({std::stod(f[0]), f[1]});
        }
        std::sort(list.begin(), list.end(), [](const Stamped& a, const Stamped& b) { return a.t < b.t; });
        return list;
    };
    const std::vector<Stamped> rgb = readList("rgb.txt");
    const std::vector<Stamped> depth = readList("depth.txt");

    std::vector<StampedPose> truth;
    {
        std::ifstream in(joinPath(directory_, "groundtruth.txt"));
        std::string line;
        while (std::getline(in, line)) {
            if (isComment(line)) continue;
            const std::vector<std::string> f = splitFields(line, ' ');
            if (f.size() < 8) continue;
            StampedPose p;
            p.timestampSeconds = std::stod(f[0]);
            p.pose.t = cv::Vec3d(std::stod(f[1]), std::stod(f[2]), std::stod(f[3]));
            p.pose.R = quaternionToRotation(std::stod(f[4]), std::stod(f[5]), std::stod(f[6]), std::stod(f[7]));
            truth.push_back(p);
        }
        std::sort(truth.begin(), truth.end(),
                  [](const StampedPose& a, const StampedPose& b) { return a.timestampSeconds < b.timestampSeconds; });
    }

    // Intrinsics published for the Kinect of each sequence family.
    const std::string name = fs::path(directory_).filename().string() + directory_;
    CameraIntrinsics k;
    k.width = 640;
    k.height = 480;
    if (name.find("freiburg1") != std::string::npos) {
        k.fx = 517.3; k.fy = 516.5; k.cx = 318.6; k.cy = 255.3;
        k.distortion = {0.2624, -0.9531, -0.0054, 0.0026, 1.1633};
    } else if (name.find("freiburg2") != std::string::npos) {
        k.fx = 520.9; k.fy = 521.0; k.cx = 325.1; k.cy = 249.7;
        k.distortion = {0.2312, -0.7849, -0.0033, -0.0001, 0.9172};
    } else if (name.find("freiburg3") != std::string::npos) {
        k.fx = 535.4; k.fy = 539.2; k.cx = 320.1; k.cy = 247.6;
    } else {
        k.fx = 525.0; k.fy = 525.0; k.cx = 319.5; k.cy = 239.5;
    }
    intrinsics_ = k;
    depthUnitsPerMetre_ = 5000.0;

    for (const Stamped& image : rgb) {
        DatasetEntry entry;
        entry.timestampSeconds = image.t;
        entry.rgbPath = joinPath(directory_, image.path);
        if (const Stamped* d = nearestInTime(depth, image.t, 0.02, [](const Stamped& s) { return s.t; }))
            entry.depthPath = joinPath(directory_, d->path);
        if (const StampedPose* p = nearestInTime(truth, image.t, 0.02,
                                               [](const StampedPose& s) { return s.timestampSeconds; })) {
            entry.hasPose = true;
            entry.pose = p->pose;
        }
        entries_.push_back(entry);
    }
    return finishOpen();
}

// ------------------------------------------------------------ EuRoC MAV

EurocFrameSource::EurocFrameSource(std::string directory, bool loop)
    : DatasetFrameSource(loop), directory_(std::move(directory)) {
    description_ = "EuRoC " + directory_;
    colourIsGrey_ = true;
}

bool EurocFrameSource::open() {
    entries_.clear();
    imu_.clear();
    const fs::path mav = fs::path(directory_) / "mav0";

    std::ifstream yamlFile(mav / "cam0" / "sensor.yaml");
    std::stringstream yaml;
    yaml << yamlFile.rdbuf();
    const std::vector<double> k = yamlNumbers(yaml.str(), "intrinsics");
    const std::vector<double> d = yamlNumbers(yaml.str(), "distortion_coefficients");
    const std::vector<double> resolution = yamlNumbers(yaml.str(), "resolution");
    const std::vector<double> bodyFromCamera = yamlNumbers(yaml.str(), "T_BS");
    if (k.size() >= 4 && resolution.size() >= 2) {
        CameraIntrinsics intrinsics;
        intrinsics.fx = k[0]; intrinsics.fy = k[1]; intrinsics.cx = k[2]; intrinsics.cy = k[3];
        for (std::size_t i = 0; i < d.size() && i < 4; ++i) intrinsics.distortion[i] = d[i];
        intrinsics.width = static_cast<int>(resolution[0]);
        intrinsics.height = static_cast<int>(resolution[1]);
        intrinsics_ = intrinsics;
    }
    RigidTransform cameraInBody;
    if (bodyFromCamera.size() >= 12) {
        cameraInBody.R = cv::Matx33d(bodyFromCamera[0], bodyFromCamera[1], bodyFromCamera[2],
                                     bodyFromCamera[4], bodyFromCamera[5], bodyFromCamera[6],
                                     bodyFromCamera[8], bodyFromCamera[9], bodyFromCamera[10]);
        cameraInBody.t = cv::Vec3d(bodyFromCamera[3], bodyFromCamera[7], bodyFromCamera[11]);
    }

    std::vector<StampedPose> truth;
    {
        std::ifstream in(mav / "state_groundtruth_estimate0" / "data.csv");
        std::string line;
        while (std::getline(in, line)) {
            const std::vector<std::string> f = splitFields(line, ',');
            if (isComment(line) || f.size() < 8) continue;
            StampedPose p;  // world_from_body; q stored w, x, y, z
            p.timestampSeconds = std::stod(f[0]) * 1e-9;
            p.pose.t = cv::Vec3d(std::stod(f[1]), std::stod(f[2]), std::stod(f[3]));
            p.pose.R = quaternionToRotation(std::stod(f[5]), std::stod(f[6]), std::stod(f[7]), std::stod(f[4]));
            truth.push_back(p);
        }
        std::sort(truth.begin(), truth.end(),
                  [](const StampedPose& a, const StampedPose& b) { return a.timestampSeconds < b.timestampSeconds; });
    }

    std::ifstream images(mav / "cam0" / "data.csv");
    std::string line;
    while (std::getline(images, line)) {
        const std::vector<std::string> f = splitFields(line, ',');
        if (isComment(line) || f.size() < 2) continue;
        DatasetEntry entry;
        entry.timestampSeconds = std::stod(f[0]) * 1e-9;
        entry.rgbPath = (mav / "cam0" / "data" / f[1]).string();
        if (const StampedPose* p = nearestInTime(truth, entry.timestampSeconds, 0.01,
                                               [](const StampedPose& s) { return s.timestampSeconds; })) {
            entry.hasPose = true;
            entry.pose = p->pose * cameraInBody;  // world_from_camera
        }
        entries_.push_back(entry);
    }

    std::ifstream imu(mav / "imu0" / "data.csv");
    while (std::getline(imu, line)) {
        const std::vector<std::string> f = splitFields(line, ',');
        if (isComment(line) || f.size() < 7) continue;
        ImuSample s;
        s.timestampSeconds = std::stod(f[0]) * 1e-9;
        s.gyro = cv::Vec3d(std::stod(f[1]), std::stod(f[2]), std::stod(f[3]));
        s.accel = cv::Vec3d(std::stod(f[4]), std::stod(f[5]), std::stod(f[6]));
        imu_.push_back(s);
    }
    return finishOpen();
}

// ------------------------------------------------------------ engine size

EngineSizeFrameSource::EngineSizeFrameSource(std::unique_ptr<IFrameSource> inner) : inner_(std::move(inner)) {}

bool EngineSizeFrameSource::read(Frame& frame) {
    if (!inner_->read(frame)) return false;
    if (frame.bgr.cols == kFrameWidth && frame.bgr.rows == kFrameHeight) return true;
    const double sx = static_cast<double>(kFrameWidth) / frame.bgr.cols;
    const double sy = static_cast<double>(kFrameHeight) / frame.bgr.rows;
    frame.bgr = toEngineFrameSize(frame.bgr);
    if (!frame.depth.empty())
        cv::resize(frame.depth, frame.depth, cv::Size(kFrameWidth, kFrameHeight), 0, 0, cv::INTER_NEAREST);
    if (frame.intrinsics) *frame.intrinsics = frame.intrinsics->scaledTo(kFrameWidth, kFrameHeight);
    for (auto& corners : frame.referenceMarkerCorners)
        for (cv::Point2f& c : corners) c = cv::Point2f(static_cast<float>(c.x * sx), static_cast<float>(c.y * sy));
    return true;
}

// ------------------------------------------------------------ writer

bool BaekarDatasetWriter::open(const std::string& directory, const std::string& device) {
    directory_ = directory;
    device_ = device;
    written_ = 0;
    anyPose_ = false;
    intrinsics_.reset();
    frameRows_.clear();
    imuRows_.clear();
    std::error_code error;
    fs::create_directories(fs::path(directory) / "rgb", error);
    if (error) {
        std::fprintf(stderr, "Record: cannot create %s: %s\n", directory.c_str(), error.message().c_str());
        return false;
    }
    return true;
}

bool BaekarDatasetWriter::write(const Frame& frame) {
    char name[32];
    std::snprintf(name, sizeof(name), "%06zu.png", written_);
    const std::string rgb = std::string("rgb/") + name;
    if (!cv::imwrite((fs::path(directory_) / rgb).string(), frame.bgr)) return false;

    std::string depth;
    if (!frame.depth.empty()) {
        std::error_code error;
        fs::create_directories(fs::path(directory_) / "depth", error);
        depth = std::string("depth/") + name;
        cv::Mat millimetres;
        frame.depth.convertTo(millimetres, CV_16U, 1000.0);
        cv::imwrite((fs::path(directory_) / depth).string(), millimetres);
    }
    if (frame.intrinsics && !intrinsics_) intrinsics_ = frame.intrinsics;

    std::ostringstream row;
    row << std::fixed << std::setprecision(9) << frame.timestampSeconds << ',' << rgb << ',' << depth;
    if (frame.referencePose) {
        anyPose_ = true;
        const cv::Vec4d q = rotationToQuaternion(frame.referencePose->R);
        const cv::Vec3d& t = frame.referencePose->t;
        row << ',' << t[0] << ',' << t[1] << ',' << t[2] << ',' << q[0] << ',' << q[1] << ',' << q[2] << ',' << q[3];
    } else {
        row << ",,,,,,,";
    }
    frameRows_.push_back(row.str());
    for (const ImuSample& s : frame.imu) {
        std::ostringstream imu;
        imu << std::fixed << std::setprecision(9) << s.timestampSeconds << ',' << s.gyro[0] << ',' << s.gyro[1]
            << ',' << s.gyro[2] << ',' << s.accel[0] << ',' << s.accel[1] << ',' << s.accel[2];
        imuRows_.push_back(imu.str());
    }
    ++written_;
    return true;
}

bool BaekarDatasetWriter::close() {
    std::ofstream frames(fs::path(directory_) / "frames.csv");
    frames << "timestamp,rgb,depth,tx,ty,tz,qx,qy,qz,qw\n";
    for (const std::string& row : frameRows_) frames << row << '\n';
    if (!imuRows_.empty()) {
        std::ofstream imu(fs::path(directory_) / "imu.csv");
        imu << "timestamp,gx,gy,gz,ax,ay,az\n";
        for (const std::string& row : imuRows_) imu << row << '\n';
    }
    cv::FileStorage meta((fs::path(directory_) / "baekar_dataset.json").string(),
                         cv::FileStorage::WRITE | cv::FileStorage::FORMAT_JSON);
    meta << "format" << "baekar-dataset";
    meta << "version" << 1;
    meta << "device" << device_;
    if (intrinsics_) {
        meta << "intrinsics" << "{";
        meta << "fx" << intrinsics_->fx << "fy" << intrinsics_->fy << "cx" << intrinsics_->cx << "cy" << intrinsics_->cy;
        meta << "width" << intrinsics_->width << "height" << intrinsics_->height;
        meta << "distortion" << "[";
        for (double v : intrinsics_->distortion) meta << v;
        meta << "]";
        meta << "}";
    }
    meta << "depth_scale" << 1000.0;
    meta << "pose_source" << (anyPose_ ? "reference" : "none");
    meta.release();
    return static_cast<bool>(frames);
}

// ------------------------------------------------------------ factory

std::unique_ptr<IFrameSource> openDatasetDirectory(const std::string& directory, bool loop) {
    const fs::path root(directory);
    std::unique_ptr<IFrameSource> source;
    if (fs::exists(root / "baekar_dataset.json"))
        source = std::make_unique<BaekarDatasetFrameSource>(directory, loop);
    else if (fs::exists(root / "rgb.txt") && fs::exists(root / "depth.txt"))
        source = std::make_unique<TumRgbdFrameSource>(directory, loop);
    else if (fs::exists(root / "mav0" / "cam0" / "data.csv"))
        source = std::make_unique<EurocFrameSource>(directory, loop);
    else if (fs::is_directory(root))
        source = std::make_unique<ImageSequenceFrameSource>(directory, loop);
    return source;
}

}  // namespace baekar
