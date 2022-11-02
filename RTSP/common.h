//
// Created by 胡启奇 on 2022/10/31.
//

#ifndef RTSP_COMMON_H
#define RTSP_COMMON_H

#include <map>
#include <string>
#include <vector>

#define SAFE_DELETE(ptr)   \
    do {                   \
        if (ptr) {         \
            delete ptr;    \
            ptr = nullptr; \
        }                  \
    } while (0);

#define SAFE_DELETE_M(ptr) \
    do {                   \
        if (ptr) {         \
            delete[] ptr;  \
            ptr = nullptr; \
        }                  \
    } while (0);

enum COMMON_CODE : int {
    CODE_CONNECT_FAIL = -8,
    CODE_INIT_NETWORK_FAIL = -7,
    CODE_INIT_RTSP_PUSHER_INIT_FAIL = -6,
    CODE_INIT_VIDEO_CAPTURE_FAIL = -5,
    CODE_INIT_VIDEO_ENCODE_FAIL = -4,
    CDE_INIT_AUDIO_CAPTURE_FAIL = -3,
    CODE_INIT_AUDIO_ENCODE_FAIL = -2,
    CODE_FAIL = -1,
    CODE_SUCCESS = 0,
    CODE_EAGAIN = 1,
    CODE_EOF = 2,
};

enum MEDIA_TYPE : int {
    MEDIA_UNKNOWN = -1,
    MEDIA_VIDEO,
    MEDIA_AUDIO
};

class Properties : public std::map<std::string, std::string> {
public:
    void SetProperty(const char *key, int intval) {
        SetProperty(std::string(key), std::to_string(intval));
    }

    void SetProperty(const char *key, uint32_t val) {
        SetProperty(std::string(key), std::to_string(val));
    }

    void SetProperty(const char *key, uint64_t val) {
        SetProperty(std::string(key), std::to_string(val));
    }

    void SetProperty(const char *key, const char *val) {
        SetProperty(std::string(key), std::string(val));
    }

    void SetProperty(const std::string &key, const std::string &val) {
        insert(std::pair<std::string, std::string>(key, val));
    }

    void GetChildren(const std::string &path, Properties &children) const {
        //Create sarch string
        std::string parent(path);
        //Add the final .
        parent += ".";
        //For each property
        for (const_iterator it = begin(); it != end(); ++it) {
            const std::string &key = it->first;
            //Check if it is from parent
            if (key.compare(0, parent.length(), parent) == 0)
                //INsert it
                children.SetProperty(key.substr(parent.length(), key.length() - parent.length()), it->second);
        }
    }

    void GetChildren(const char *path, Properties &children) const {
        GetChildren(std::string(path), children);
    }

    Properties GetChildren(const std::string &path) const {
        Properties properties;
        //Get them
        GetChildren(path, properties);
        //Return
        return properties;
    }

    Properties GetChildren(const char *path) const {
        Properties properties;
        //Get them
        GetChildren(path, properties);
        //Return
        return properties;
    }

    void GetChildrenArray(const char *path, std::vector<Properties> &array) const {
        //Create sarch string
        std::string parent(path);
        //Add the final .
        parent += ".";

        //Get array length
        int length = GetProperty(parent + "length", 0);

        //For each element
        for (int i = 0; i < length; ++i) {
            char index[64];
            //Print string
            snprintf(index, sizeof(index), "%d", i);
            //And get children
            array.push_back(GetChildren(parent + index));
        }
    }

    const char *GetProperty(const char *key) const {
        return GetProperty(key, "");
    }

    std::string GetProperty(const char *key, const std::string defaultValue) const {
        //Find item
        const_iterator it = find(std::string(key));
        //If not found
        if (it == end())
            //return default
            return defaultValue;
        //Return value
        return it->second;
    }

    std::string GetProperty(const std::string &key, const std::string defaultValue) const {
        //Find item
        const_iterator it = find(key);
        //If not found
        if (it == end())
            //return default
            return defaultValue;
        //Return value
        return it->second;
    }

    const char *GetProperty(const char *key, const char *defaultValue) const {
        //Find item
        const_iterator it = find(std::string(key));
        //If not found
        if (it == end())
            //return default
            return defaultValue;
        //Return value
        return it->second.c_str();
    }

    const char *GetProperty(const std::string &key, char *defaultValue) const {
        //Find item
        const_iterator it = find(key);
        //If not found
        if (it == end())
            //return default
            return defaultValue;
        //Return value
        return it->second.c_str();
    }

    int GetProperty(const char *key, int defaultValue) const {
        return GetProperty(std::string(key), defaultValue);
    }

    int GetProperty(const std::string &key, int defaultValue) const {
        //Find item
        const_iterator it = find(key);
        //If not found
        if (it == end())
            //return default
            return defaultValue;
        //Return value
        return atoi(it->second.c_str());
    }

    uint64_t GetProperty(const char *key, uint64_t defaultValue) const {
        return GetProperty(std::string(key), defaultValue);
    }

    uint64_t GetProperty(const std::string &key, uint64_t defaultValue) const {
        //Find item
        const_iterator it = find(key);
        //If not found
        if (it == end())
            //return default
            return defaultValue;
        //Return value
        return atoll(it->second.c_str());
    }

    bool GetProperty(const char *key, bool defaultValue) const {
        return GetProperty(std::string(key), defaultValue);
    }

    bool GetProperty(const std::string &key, bool defaultValue) const {
        //Find item
        const_iterator it = find(key);
        //If not found
        if (it == end())
            //return default
            return defaultValue;
        //Get value
        char *val = (char *) it->second.c_str();
        //Check it
        if (strcasecmp(val, (char *) "yes") == 0)
            return true;
        else if (strcasecmp(val, (char *) "true") == 0)
            return true;
        //Return value
        return (atoi(val));
    }
};

struct VideoCaptureParam {
    int width_;
    int height_;
    int fps_;
    std::string fmt_name_;
    std::string pix_fmt_;

    VideoCaptureParam() : width_(1280), height_(720), fps_(24), fmt_name_("avfoundation"), pix_fmt_("yuyv422") {}

    Properties GetProperties() {
        Properties res;
        res.SetProperty("width", this->width_);
        res.SetProperty("height", this->height_);
        res.SetProperty("fps", this->fps_);
        res.SetProperty("fmt_name", this->fmt_name_);
        res.SetProperty("pix_fmt", this->pix_fmt_);
        return res;
    }
};

struct AudioCaptureParam {
};

struct VideoEncodeParam {
    int width_;
    int height_;
    int fps_;
    int bFrames_;
    int bitrate_;
    int gop_;
    int thread_num_;
    int pix_fmt_;
    int qp_min_;
    int qp_max_;
    std::string codec_name_;
    std::string preset_;
    std::string tune_;
    std::string profile_;

    VideoEncodeParam() : width_(0), height_(0), fps_(0), bFrames_(0), bitrate_(0),
                         gop_(0), thread_num_(0), pix_fmt_(-1), qp_min_(0), qp_max_(0),
                         codec_name_("default"), preset_("medium"),
                         tune_("zerolatency"), profile_("high") {}

    Properties GetProperties() {
        Properties res;
        res.SetProperty("width", this->width_);
        res.SetProperty("height", this->height_);
        res.SetProperty("fps", this->fps_);
        res.SetProperty("bFrames", this->bFrames_);
        res.SetProperty("bitrate", this->bitrate_);
        res.SetProperty("pix_fmt", this->pix_fmt_);
        res.SetProperty("qp_min", this->qp_min_);
        res.SetProperty("qp_max", this->qp_max_);
        res.SetProperty("codec_name", this->codec_name_);
        res.SetProperty("preset", this->preset_);
        res.SetProperty("tune", this->tune_);
        res.SetProperty("profile", this->profile_);
        return res;
    }
};

struct AudioEncodeParam {
};

struct RtspPusherParam {
    std::string url_;
    std::string rtsp_transport_;
    int timeout_;//单位：s
    int maxQueueCapacity_;

    RtspPusherParam() : url_(""), rtsp_transport_("tcp"), timeout_(5), maxQueueCapacity_(500) {}

    Properties GetProperties() {
        Properties res;
        res.SetProperty("url", this->url_);
        res.SetProperty("rtsp_transport", this->rtsp_transport_);
        res.SetProperty("timeout", this->timeout_);
        res.SetProperty("maxQueueCapacity", this->maxQueueCapacity_);
        return res;
    }
};

struct PushStreamParam {
    VideoCaptureParam vCaptureParam_;
    VideoEncodeParam vEncodeParam_;
    AudioCaptureParam aCaptureParam_;
    AudioEncodeParam aEncodeParam_;
    RtspPusherParam rtspParam_;
};

#endif//RTSP_COMMON_H
