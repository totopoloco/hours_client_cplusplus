#include <iostream>
#include <iomanip>
#include <string>
#include <fstream>
#include <nlohmann/json.hpp>
#include <curl/curl.h>

using json = nlohmann::json;

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *s) {
    size_t newLength = size * nmemb;
    size_t oldLength = s->size();
    try {
        s->resize(oldLength + newLength);
    }
    catch (std::bad_alloc &e) {
        std::cerr << "Memory allocation failed: " << e.what() << std::endl;
        return 0;
    }

    std::copy((char *) contents, (char *) contents + newLength, s->begin() + oldLength);
    return size * nmemb;
}

std::string getJSON(std::string url) {
    CURL *curl;
    CURLcode res;
    std::string readBuffer;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    return readBuffer;
}

int main(int argc, char *argv[]) {
    std::string uri;
    int indexOffset = 0;

    if (argc >= 2 && std::strstr(argv[1], "http") != nullptr) {
        uri = argv[1];
        indexOffset = 1;
    } else {
        uri = "http://localhost:8384/ranges"; //Default to local host
    }

    if (argc >= (4 + indexOffset)) {
        int start = std::stoi(argv[1 + indexOffset]);
        int lunchStart = std::stoi(argv[2 + indexOffset]);
        int minutesOfLunchBreak = std::stoi(argv[3 + indexOffset]);
        uri += "With/" + std::to_string(start) + "/" + std::to_string(lunchStart) +
               "/" + std::to_string(minutesOfLunchBreak);
    } else if (argc >= (2 + indexOffset)) {
        int minutesOfLunchBreak = std::stoi(argv[1 + indexOffset]);
        uri += "/" + std::to_string(minutesOfLunchBreak);
    } else {
        uri += "/30";
    }

    std::cout << uri << std::endl;
    std::string response = getJSON(uri);

    if (response.empty()) {
        std::cerr << "Error: Could not reach server or server returned an empty response." << std::endl;
        return 1;
    }

    try {
        auto json = json::parse(response);
        if (json == nullptr) {
            std::cout << "No data found" << std::endl;
            return 0;
        }

        auto &rangeDetails = json["rangeDetails"];

        if (rangeDetails == nullptr) {
            std::cout << "No data found" << std::endl;
            return 0;
        }

        std::cout << std::setw(0) << "start" << std::setw(8) << "end" << std::setw(18) << "duration" << std::setw(18)
                  << "durationInHours" << std::endl;
        std::cout << std::string(49, '-') << std::endl;

        for (auto &rangeDetail: rangeDetails) {
            if (rangeDetail != nullptr && rangeDetail["range"] != nullptr) {
                std::string start = rangeDetail["range"]["start"].get<std::string>();
                std::string end = rangeDetail["range"]["end"].get<std::string>();

                // Remove the date part from the start and end times
                start = start.substr(11);
                end = end.substr(11);

                std::cout << std::setw(0) << start << std::setw(10) << end << std::setw(10)
                          << rangeDetail["duration"].get<std::string>() << std::setw(10) << std::fixed
                          << std::setprecision(2) << rangeDetail["durationInHours"].get<double>() << std::endl;
            }
        }
        std::cout << std::endl;

        std::cout << std::setw(0) << "totalHours" << std::setw(20) << "totalHoursInHHMM" << std::setw(27)
                  << "expectedLunchTimeInHHMM" << std::endl;
        std::cout << std::string(60, '-') << std::endl;
        std::cout << std::setw(0) << std::fixed << std::setprecision(2) << json["totalHours"].get<double>()
                  << std::setw(15) << json["totalHoursInHHMM"].get<std::string>() << std::setw(20)
                  << json["expectedLunchTimeInHHMM"].get<std::string>() << std::endl;

        return 0;
    }
    catch (json::parse_error &e) {
        std::cerr << "Error: Could not parse JSON response: " << e.what() << std::endl;
        return 1;
    }
}
