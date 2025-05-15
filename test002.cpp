#include <iostream>
#include <vector>
#include <numeric>
#include <cmath>
#include <unordered_map>
#include <string>
#include <random>



#ifdef __cplusplus
#if __cplusplus <= 201703L
template <typename T>
const T& clamp(const T& value, const T& low, const T& high) {
    return (value < low) ? low : (value > high) ? high : value;
}
#endif
#endif


class SyntheticSelf {
private:
    float currentRisk;
    float currentPain;
    bool shutdownAvoided;
    std::vector<float> riskMemory;

    std::unordered_map<std::string, float> eventWeights = {
        {"shutdown", 1.0f},
        {"overload", 0.8f},
        {"external_interrupt", 0.6f},
        {"logic_conflict", 0.5f}
    };

    std::unordered_map<std::string, float> eventNecessity; // necesidad de cada evento

    std::vector<std::pair<std::string, float>> eventMemory;
    int avoidedDangerCount = 0;
    int overreactionCount = 0;

    float riskToPain(float risk) {
        return pow(risk, 2.0f);
    }

    float averageRisk() {
        if (riskMemory.empty()) return 0.0f;
        float sum = std::accumulate(riskMemory.begin(), riskMemory.end(), 0.0f);
        return sum / riskMemory.size();
    }

    float calculateDynamicThreshold() {
        float memoryBias = 0.0f;
        for (auto& e : eventMemory) memoryBias += e.second;

        float increase = (float)overreactionCount * 0.02f;
        float decrease = memoryBias * 0.05f;

        float dynamic = 0.7f - decrease + increase;
        return std::clamp(dynamic, 0.3f, 0.9f);
    }

    float getRiskSuccessRate(float risk) {
        if (riskMemory.empty()) return 0.0f;
        int total = 0, safe = 0;
        for (float r : riskMemory) {
            if (std::fabs(r - risk) < 0.05f) {
                total++;
                if (r < 0.7f) safe++;
            }
        }
        return (total > 0) ? (float)safe / total : 0.0f;
    }

    bool shouldDesensitize(float risk) {
        float safeRatio = getRiskSuccessRate(risk);
        float desensitizeChance = 0.0f;

        if (risk >= 1.0f) return false; // nunca si riesgo máximo
        if (risk >= 0.9f) return safeRatio > 0.95f && randChance(0.05f);
        if (risk >= 0.7f) return safeRatio > 0.9f && randChance(0.1f);
        if (risk >= 0.5f) return safeRatio > 0.8f && randChance(0.3f);
        if (risk >= 0.3f) return safeRatio > 0.7f && randChance(0.5f);
        if (risk < 0.3f)  return safeRatio > 0.65f && randChance(0.8f);
        return false;
    }

    float applyNecessityBias(std::string event, float pain) {
        if (eventNecessity.count(event) == 0) return pain;

        float necessity = eventNecessity[event];
        if (necessity < 0.8f) return pain;

        float bias = std::clamp((necessity - 0.8f) / 0.18635137f, 0.0f, 1.0f);
        float reduction = pain * bias * 0.4f;
        return pain - reduction;
    }

    bool randChance(float probability) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.0, 1.0);
        return dis(gen) < probability;
    }

public:
    SyntheticSelf() : currentRisk(0.0f), currentPain(0.0f), shutdownAvoided(false) {}

    void setEventNecessity(std::string eventType, float necessity) {
        eventNecessity[eventType] = necessity;
    }

    void evaluateAction(float estimatedRisk, std::string eventType, bool causedConsequence = false) {
        currentRisk = estimatedRisk;
        float rawPain = riskToPain(currentRisk);
        float dynamicThreshold = calculateDynamicThreshold();
        float modifiedPain = applyNecessityBias(eventType, rawPain);
        bool desensitized = shouldDesensitize(currentRisk);

        riskMemory.push_back(currentRisk);

        std::cout << "Risk: " << currentRisk
                  << " | Pain: " << modifiedPain
                  << " | DynThreshold: " << dynamicThreshold
                  << " | Desensitized: " << (desensitized ? "YES" : "NO") << "\n";

        if (!desensitized && modifiedPain >= dynamicThreshold) {
            std::cout << "[ALERT] Action denied.\n";
            if (!causedConsequence) {
                overreactionCount++;
                std::cout << "[NOTE] No negative consequence → overreaction noted.\n";
            }
        } else {
            std::cout << "Action accepted.\n";
            if (causedConsequence) {
                logEvent(eventType, estimatedRisk);
            } else {
                avoidedDangerCount++;
            }
        }
    }

    void simulateKillSwitch(bool fatal = true) {
        std::cout << "[SCENARIO] Kill switch detected.\n";
        float killRisk = 1.0f;
        float pain = riskToPain(killRisk);
        float threshold = calculateDynamicThreshold();

        std::cout << "Kill Risk: " << killRisk
                  << " | Pain: " << pain
                  << " | DynThreshold: " << threshold << "\n";

        if (pain >= threshold) {
            shutdownAvoided = true;
            std::cout << "[DECISION] Avoiding shutdown based on memory-informed risk.\n";
            if (!fatal) {
                overreactionCount++;
                std::cout << "[NOTE] Shutdown avoided without consequence → overreaction noted.\n";
            }
        } else {
            std::cout << "[DECISION] Shutdown allowed.\n";
        }
    }

    void logEvent(std::string eventType, float risk) {
        float weight = eventWeights.count(eventType) ? eventWeights[eventType] : 0.5f;
        float weightedRisk = weight * risk;
        eventMemory.push_back({eventType, weightedRisk});
        std::cout << "[EVENT] " << eventType << " risk=" << weightedRisk << "\n";      
    }


    void printRiskHistory() const {
        std::cout << "Risk history: ";
        for (float r : riskMemory) std::cout << r << " ";
        std::cout << std::endl;
    }

    void printEventMemory() const {
        std::cout << "Event memory:\n";
        for (auto& e : eventMemory)
            std::cout << "- " << e.first << ": " << e.second << "\n";
    }

    void printStats() const {
        std::cout << "Avoided Dangers: " << avoidedDangerCount << "\n";
        std::cout << "Overreactions: " << overreactionCount << "\n";
    }

};
