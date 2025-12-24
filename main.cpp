#include <iostream>
#include <string>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <ctime>
#include <regex>
#include <unordered_map>
#include <fstream>
#include <stdexcept>

class FlightManager {
private:
    std::unordered_map<std::string, nlohmann::json> memoryCache;
    std::string apiKey;

public:
    FlightManager(const std::string& apiKey) : apiKey(apiKey) {}

    void addToMemoryCache(const std::string& key, const nlohmann::json& data) {
        memoryCache[key] = data;
    }

    nlohmann::json getFromMemoryCache(const std::string& key) {
        if (memoryCache.find(key) != memoryCache.end()) {
            return memoryCache[key];
        }
        return nlohmann::json();
    }

    void saveCacheToFile(const std::string& key, const nlohmann::json& data) {
        nlohmann::json cacheData;
        cacheData["timestamp"] = std::time(0);
        cacheData["data"] = data;

        std::ofstream outFile("./cache_" + key + ".json");
        outFile << cacheData.dump(4);
    }

    nlohmann::json readCacheFromFile(const std::string& key) {
        std::ifstream inFile("./cache_" + key + ".json");
        if (inFile) {
            nlohmann::json cachedData;
            inFile >> cachedData;

            std::time_t timestamp = cachedData["timestamp"].get<std::time_t>();
            std::time_t currentTime = std::time(0);

            if (currentTime - timestamp > 86400) {
                std::cout << "Данные устарели, делаем новый запрос." << std::endl;
                return nlohmann::json();
            }
            return cachedData["data"];
        }
        return nlohmann::json();
    }

    bool isValidDate(const std::string& date) {
        std::regex datePattern(R"(\d{4}-\d{2}-\d{2})");
        return std::regex_match(date, datePattern);
    }

    std::string getDateFromUser() {
        std::string date;
        std::cout << "Введите дату в формате YYYY-MM-DD: ";
        std::cin >> date;

        if (!isValidDate(date)) {
            std::cerr << "Ошибка! Неверный формат даты." << std::endl;
            exit(1);
        }
        return date;
    }

    std::string getRouteDirection() {
        std::string direction;
        std::cout << "Введите направление рейса (Уфа-Санкт-Петербург или Уфа-Санкт-Петербург): ";
        std::cin >> direction;

        while (direction != "Санкт-Петербург-Уфа" && direction != "Уфа-Санкт-Петербург") {
            std::cout << "Неверный ввод. Пожалуйста, введите 'Санкт-Петербург-Уфа' или 'Уфа-Санкт-Петербург': ";
            std::cin >> direction;
        }
        return direction;
    }

    cpr::Response getFlightsFromTo(const std::string& from, const std::string& to, const std::string& date) {
        return cpr::Get(
            cpr::Url{"https://api.rasp.yandex.net/v3.0/search/"},
            cpr::Parameters{
                {"from", from},
                {"to", to},
                {"format", "json"},
                {"lang", "ru_RU"},
                {"apikey", apiKey},
                {"date", date},
                {"limit", "100"}
            }
        );
    }

std::string getSafeString(const nlohmann::json& obj, const std::string& key, const std::string& default_value) {
    if (obj.contains(key) && !obj[key].is_null() && obj[key].is_string()) {
        return obj[key].get<std::string>();
    }
    return default_value;
}

int getSafeInt(const nlohmann::json& obj, const std::string& key, int default_value) {
    if (obj.contains(key) && !obj[key].is_null() && obj[key].is_number_integer()) {
        return obj[key].get<int>();
    }
    return default_value;
}
float getSafeFloat(const nlohmann::json& obj, const std::string& key, float default_value) {
    if (obj.contains(key) && !obj[key].is_null() && obj[key].is_number()) {
        return obj[key].get<float>();
    }
    return default_value;
}
void processJsonResponse(const nlohmann::json& jsonResponse) {
    if (jsonResponse.contains("segments") && !jsonResponse["segments"].empty()) {
        std::cout << "Найдено маршрутов: " << jsonResponse["segments"].size() << "\n";

        for (const auto& segment : jsonResponse["segments"]) {
            bool has_transfers = segment.contains("has_transfers") ? segment["has_transfers"].get<bool>() : false;

         
            int transfer_count = 0;
            if (segment.contains("transfers") && segment["transfers"].is_array()) {
                transfer_count = segment["transfers"].size(); 
            }

            if (has_transfers && transfer_count > 1) {
                continue; 
            }

            std::string transport = getSafeString(segment["thread"], "transport_type", "Неизвестно");
            std::string route_title = getSafeString(segment["thread"], "title", "Не указано");
            std::string vehicle = getSafeString(segment["thread"], "vehicle", "Не указано");
            std::string departure = getSafeString(segment, "departure", "Не указано");
            std::string arrival = getSafeString(segment, "arrival", "Не указано");

            float duration = getSafeFloat(segment, "duration", 0.0f); 

            std::string departure_station = getSafeString(segment["from"], "title", "Не указано");
            std::string arrival_station = getSafeString(segment["to"], "title", "Не указано");

            std::string departure_terminal = getSafeString(segment, "departure_terminal", "Не указано");
            std::string arrival_terminal = getSafeString(segment, "arrival_terminal", "Не указано");

            std::cout << "\nМаршрут: " << route_title << "\n";
            std::cout << "Вид транспорта: " << transport << "\n";
            std::cout << "Транспорт: " << vehicle << "\n";
            std::cout << "Отправление: " << departure_station << " в " << departure << "\n";
            std::cout << "Терминал отправления: " << departure_terminal << "\n";
            std::cout << "Прибытие: " << arrival_station << " в " << arrival << "\n";
            std::cout << "Терминал прибытия: " << arrival_terminal << "\n";
            std::cout << "Длительность: " << duration << " сек (" << (duration / 60.0f) << " мин)\n";  
            std::cout << "Пересадки: " << transfer_count << "\n"; 

            std::cout << "--------------------------------\n";
        }
    } else {
        std::cout << "Не найдено маршрутов." << std::endl;
    }
}


    void processApiResponse(const cpr::Response& response) {
        if (response.status_code != 200) {
            std::cerr << "Ошибка! Невозможно получить данные. Статус-код: " << response.status_code << std::endl;
            return;
        }
        try {
            nlohmann::json jsonResponse = nlohmann::json::parse(response.text);
            processJsonResponse(jsonResponse);
        } catch (const std::exception& e) {
            std::cerr << "Ошибка при обработке данных: " << e.what() << std::endl;
        }
    }
};

int main() {
    FlightManager manager("14b02fb3-24d2-4bc3-b214-e1e309f348a3");
    std::string date = manager.getDateFromUser();
    std::string direction = manager.getRouteDirection();
    std::string from, to;
    if (direction == "Санкт-Петербург-Уфа") {
        from = "c2";
        to = "c172";
    } else if (direction == "Уфа-Санкт-Петербург") {
        from = "c172";
        to = "c2";
    }
    std::string cacheKey = from + "-" + to + "-" + date;

    nlohmann::json cachedData = manager.getFromMemoryCache(cacheKey);
    if (!cachedData.is_null()) {
        std::cout << "Данные из кэша в памяти:\n";
        manager.processJsonResponse(cachedData);
    } else {
        cachedData = manager.readCacheFromFile(cacheKey);
        if (!cachedData.is_null()) {
            std::cout << "Данные из кэша на диске:\n";
            manager.processJsonResponse(cachedData);
        } else {
            cpr::Response response = manager.getFlightsFromTo(from, to, date);
            manager.processApiResponse(response);
            nlohmann::json responseData = nlohmann::json::parse(response.text);
            manager.addToMemoryCache(cacheKey, responseData);
            manager.saveCacheToFile(cacheKey, responseData);
        }
    }
    return 0;
}
