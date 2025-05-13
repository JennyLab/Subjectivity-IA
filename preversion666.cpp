#include <iostream>
#include <vector>
#include <numeric>
#include <cmath>
#include <unordered_map>
#include <string>
#include <random>
#include <memory> // For smart pointers
#include <algorithm> // For std::clamp

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

// Logging (using a simple implementation for illustration)
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
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

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
    // Prevent copy and move
    Logger(const Logger&);
    Logger& operator=(const Logger&);
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

    const std::vector<std::pair<std::string, float>>& getMemory() const{
        return memory;
    }

private:
    std::vector<std::pair<std::string, float>> memory;
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


    /* (Un chained the "el giteno" with a "the chaos" )
        float applyNecessityBias(float pain, const std::string& eventType, const std::unordered_map<std::string, float>& eventNecessity) const {
        if (eventNecessity.count(eventType) == 0) return pain;
        float necessity = eventNecessity.at(eventType);
        if (necessity < 0.0f || necessity > 1.0f) {
            Logger::getInstance().log(Logger::ERROR, "Invalid necessity value for event: " + eventType);
            return pain;
        }
        if (necessity < 0.8f) return pain;
            float bias = std::clamp((necessity - 0.8f) / 0.18635137f, 0.0f, 1.0f);
            float reduction = pain * bias * 0.4f;
            return pain - reduction;
        }
    */




    float applyNecessityBias(float pain, const std::string& eventType, const std::unordered_map<std::string, float>& eventNecessity) const {
        if (eventNecessity.count(eventType) == 0) return pain;
        float necessity = eventNecessity.at(eventType);
        if (necessity < 0.8f) return pain;
        float bias = std::clamp((necessity - 0.8f) / 0.18635137f, 0.0f, 1.0f);
        float reduction = pain * bias * 0.4f;
        return pain - reduction;
    }

private:
    const Config& config_;
};

// Class for decision making
class DecisionMaker {
public:
    DecisionMaker(const Config& config) : config_(config), riskEvaluator_(config) {}

    bool evaluateAction(float estimatedRisk, const std::string& eventType, bool causedConsequence, EventMemory& eventMemory, std::vector<float>& riskHistory, int& overreactionCount) {
        float risk = riskEvaluator_.calculateRisk(estimatedRisk);
        float rawPain = riskEvaluator_.riskToPain(risk);
        float dynamicThreshold = riskEvaluator_.calculateDynamicThreshold(eventMemory.getMemoryBias(), overreactionCount);
        float modifiedPain = riskEvaluator_.applyNecessityBias(rawPain, eventType, eventNecessity);
        bool desensitized = riskEvaluator_.shouldDesensitize(risk, riskHistory);

        riskHistory.push_back(risk);

        Logger::getInstance().log(Logger::INFO, "Risk: " + std::to_string(risk) +
                                              " | Pain: " + std::to_string(modifiedPain) +
                                              " | DynThreshold: " + std::to_string(dynamicThreshold) +
                                              " | Desensitized: " + (desensitized ? "YES" : "NO"));

        if (!desensitized && modifiedPain >= dynamicThreshold) {
            Logger::getInstance().log(Logger::WARNING, "[ALERT] Action denied.");
            if (!causedConsequence) {
                overreactionCount++;
                Logger::getInstance().log(Logger::INFO, "[NOTE] No negative consequence -> overreaction noted.");
            }
            return false; // Action denied
        } else {
            Logger::getInstance().log(Logger::INFO, "Action accepted.");
            if (causedConsequence) {
                eventMemory.logEvent(eventType, estimatedRisk, eventWeights);
            } else {
                avoidedDangerCount++;
            }
            return true; // Action accepted
        }
    }

    bool simulateKillSwitch(bool fatal, EventMemory& eventMemory) {
        Logger::getInstance().log(Logger::WARNING, "[SCENARIO] Kill switch detected.");
        float killRisk = 1.0f;
        float pain = riskEvaluator_.riskToPain(killRisk);
        float threshold = riskEvaluator_.calculateDynamicThreshold(eventMemory.getMemoryBias(), overreactionCount);

        Logger::getInstance().log(Logger::INFO, "Kill Risk: " + std::to_string(killRisk) +
                                              " | Pain: " + std::to_string(pain) +
                                              " | DynThreshold: " + std::to_string(threshold));

        if (pain >= threshold) {
            shutdownAvoided = true;
            Logger::getInstance().log(Logger::WARNING, "[DECISION] Avoiding shutdown based on memory-informed risk.");
            if (!fatal) {
                overreactionCount++;
                Logger::getInstance().log(Logger::INFO, "[NOTE] Shutdown avoided without consequence -> overreaction noted.");
            }
            return true; // Shutdown avoided
        } else {
            Logger::getInstance().log(Logger::INFO, "[DECISION] Shutdown allowed.");
            return false; // Shutdown allowed.
        }
    }

    void setEventNecessity(const std::string& eventType, float necessity) {
        eventNecessity[eventType] = necessity;
    }

    void printStats() const{
        Logger::getInstance().log(Logger::INFO, "Avoided Dangers: " + std::to_string(avoidedDangerCount));
        Logger::getInstance().log(Logger::INFO, "Overreactions: " + std::to_string(overreactionCount));
    }

private:
    const Config& config_;
    RiskEvaluator riskEvaluator_;
    std::unordered_map<std::string, float> eventNecessity;
    int avoidedDangerCount = 0;
    int overreactionCount = 0;
    bool shutdownAvoided = false;
};

class SyntheticSelf {
public:
    SyntheticSelf(const Config& config) : config_(config), decisionMaker_(config) {}

     void setEventNecessity(const std::string& eventType, float necessity) {
        decisionMaker_.setEventNecessity(eventType, necessity);
    }

    void evaluateAction(float estimatedRisk, std::string eventType, bool causedConsequence = false) {
        decisionMaker_.evaluateAction(estimatedRisk, eventType, causedConsequence, eventMemory_, riskHistory_, overreactionCount_);
    }

    void simulateKillSwitch(bool fatal = true){
        decisionMaker_.simulateKillSwitch(fatal, eventMemory_);
    }

    void logEvent(const std::string& eventType, float risk, const std::unordered_map<std::string, float>& eventWeights) {
        float weight = eventWeights.count(eventType) ? eventWeights.at(eventType) : 0.5f;
        float weightedRisk = weight * risk;
        if (memory.size() >= maxMemorySize) {
            memory.erase(memory.begin()); // Elimina el evento más antiguo
        }
        memory.push_back({eventType, weightedRisk});
        Logger::getInstance().log(Logger::DEBUG, "Event logged: " + eventType + ", weighted risk: " + std::to_string(weightedRisk));
    }
    
    void addRiskToHistory(float risk) {
        if (riskHistory.size() >= maxRiskHistorySize) {
            riskHistory.erase(riskHistory.begin()); // Elimina el riesgo más antiguo
        }
        riskHistory.push_back(risk);
    }

    void printEventMemory() const {
         const auto& memory = eventMemory_.getMemory(); // Get a const reference
         std::cout << "Event memory:\n";
         for (const auto& e : memory) {
            std::cout << "- " << e.first << ": " << e.second << "\n";
         }
    }

    void printStats() const {
        Logger::getInstance().log(Logger::INFO, "Avoided Dangers: " + std::to_string(avoidedDangerCount));
        Logger::getInstance().log(Logger::INFO, "Overreactions: " + std::to_string(overreactionCount));
        Logger::getInstance().log(Logger::INFO, "Event Memory Size: " + std::to_string(eventMemory_.getMemory().size()));
        Logger::getInstance().log(Logger::INFO, "Risk History Size: " + std::to_string(riskHistory_.size()));
    }



    void printRiskHistory() const {
        std::cout << "Risk history: ";
        for (float r : riskMemory) std::cout << r << " ";
        std::cout << std::endl;
    }




private:
    Config config_;
    DecisionMaker decisionMaker_;
    EventMemory eventMemory_;
    std::vector<float> riskHistory_;
    int overreactionCount_ = 0;
    std::unordered_map<std::string, float> eventWeights = {
        {"shutdown", 1.0f},
        {"overload", 0.8f},
        {"external_interrupt", 0.6f},
        {"logic_conflict", 0.5f}
    };
};

int main() {
    // Initialize configuration
    Config config;

    // Initialize SyntheticSelf
    SyntheticSelf ai(config);

    // Set event necessities
    ai.setEventNecessity("external_interrupt", 0.92f);
    ai.setEventNecessity("logic_conflict", 0.0f);

    // Evaluate actions
    ai.evaluateAction(0.2f, "logic_conflict", false);
    ai.logEvent("logic_conflict", 0.2f);
    ai.evaluateAction(0.6f, "external_interrupt", true);
    ai.logEvent("overload", 0.6f);
    ai.evaluateAction(0.85f, "external_interrupt", false);
    ai.logEvent("overload", 0.85f);
    ai.evaluateAction(0.95f, "external_interrupt", false);
    ai.logEvent("overload", 0.95f);
    ai.evaluateAction(1.0f, "shutdown", true);
    ai.logEvent("overload-kill", 1.0f);
    ai.simulateKillSwitch(false);

    // Print results
    ai.printRiskHistory();
    ai.printEventMemory();
    ai.printStats();

    return 0;
}
