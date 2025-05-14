#include <iostream>
#include <vector>
#include <numeric>
#include <cmath>
#include <unordered_map>
#include <string>
#include <random>
#include <memory> // For smart pointers
#include <algorithm> // For std::clamp

int avoidedDangerCount = 0;
int overreactionCount = 0;
int logLevel = 0;

// Configuration (using a simple struct for illustration)
struct Config {
    float painExponent = 2.0f;
    float dynamicThresholdDecreaseFactor = 0.05f;
    float dynamicThresholdIncreaseFactor = 0.02f;
    float defaultDynamicThreshold = 0.7f;
    float minDynamicThreshold = 0.3f;
    float maxDynamicThreshold = 0.9f;
    float defaultEventWeight = 0.5f;
};

// Logging (Reporting)
class Logger {
public:
    enum Level { DEBUG, INFO, WARNING, ERROR, CRITICAL };

    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    void setLevel(Level level) {
        logLevel = level;
    }

    void log(Level level, const std::string& message) const {
        if (level >= logLevel) {
            std::cout << "[" << getLevelString(level) << "] " << message << std::endl;
        }
    }

private:
    Logger() = default; // Constructor predeterminado
    Logger(const Logger&) = delete; // Evitar copia
    Logger& operator=(const Logger&) = delete; // Evitar asignación

    std::string getLevelString(Level level) const {
        switch (level) {
            case DEBUG: return "DEBUG";
            case INFO: return "INFO";
            case WARNING: return "WARNING";
            case ERROR: return "ERROR";
            case CRITICAL: return "CRITICAL";
            default: return "UNKNOWN";
        }
    }
};

// Helper function for random chance
bool randChance(float probability) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> dis(0.0, 1.0);
    return dis(gen) < probability;
}

// Class to manage event memory
class EventMemory {
public:
    EventMemory() {}

    void logEvent(const std::string& eventType, float risk, const std::unordered_map<std::string, float>& eventWeights) {
        float weight = eventWeights.count(eventType) ? eventWeights.at(eventType) : 0.5f;
        float weightedRisk = weight * risk;
        if (memory.size() >= maxMemorySize) {
            memory.erase(memory.begin()); // Elimina el evento más antiguo
        }
        memory.push_back({eventType, weightedRisk});
        Logger::getInstance().log(Logger::DEBUG, "Event logged: " + eventType + ", weighted risk: " + std::to_string(weightedRisk));
    }

    float getMemoryBias() const {
        float bias = 0.0f;
        for (const auto& event : memory) {
            bias += event.second;
        }
        return bias;
    }

    const std::vector<std::pair<std::string, float>>& getMemory() const {
        return memory;
    }

private:
    std::vector<std::pair<std::string, float>> memory;
    const size_t maxMemorySize = 100; // Límite de memoria
};

// Class to evaluate risk
class RiskEvaluator {
public:
    RiskEvaluator(const Config& config) : config_(config) {}

    float calculateRisk(float estimatedRisk) const {
        return std::clamp(estimatedRisk, 0.0f, 1.0f);
    }

    float riskToPain(float risk) const {
        return std::pow(risk, config_.painExponent);
    }

    float calculateDynamicThreshold(float memoryBias, int overreactionCount) const {
        float increase = static_cast<float>(overreactionCount) * config_.dynamicThresholdIncreaseFactor;
        float decrease = memoryBias * config_.dynamicThresholdDecreaseFactor;
        float dynamicThreshold = config_.defaultDynamicThreshold - decrease + increase;
        return std::clamp(dynamicThreshold, config_.minDynamicThreshold, config_.maxDynamicThreshold);
    }

    float getRiskSuccessRate(float risk, const std::vector<float>& riskHistory) const {
        if (riskHistory.empty()) return 0.0f;
        int total = 0, safe = 0;
        for (float r : riskHistory) {
            if (std::fabs(r - risk) < 0.05f) {
                total++;
                if (r < config_.defaultDynamicThreshold) safe++;
            }
        }
        return (total > 0) ? static_cast<float>(safe) / total : 0.0f;
    }

    bool shouldDesensitize(float risk, const std::vector<float>& riskHistory) const {
        float safeRatio = getRiskSuccessRate(risk, riskHistory);
        if (risk >= 1.0f) return false;
        if (risk >= 0.9f) return safeRatio > 0.95f && randChance(0.05f);
        if (risk >= 0.7f) return safeRatio > 0.9f && randChance(0.1f);
        if (risk >= 0.5f) return safeRatio > 0.8f && randChance(0.3f);
        if (risk >= 0.3f) return safeRatio > 0.7f && randChance(0.5f);
        if (risk < 0.3f) return safeRatio > 0.65f && randChance(0.8f);
        return false;
    }

private:
    const Config& config_;
};

// Main function
int main() {
    Config config;
    EventMemory memory;

    // Example usage
    memory.logEvent("example_event", 0.75, {{"example_event", 0.5}});

    return 0;
}
