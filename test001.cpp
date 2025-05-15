#include <iostream>
#include <vector>
#include <numeric>
#include <cmath>
#include <unordered_map>
#include <string>
#include <random>
#include <algorithm>


#ifdef __cplusplus
#if __cplusplus <= 201703L
template <typename T>
const T& clamp(const T& value, const T& low, const T& high) {
    return (value < low) ? low : (value > high) ? high : value;
}
#endif
#endif

/**
 * @class SyntheticSelf
 * @brief A class that simulates a synthetic entity capable of evaluating risks, 
 *        managing memory, and making decisions based on dynamic thresholds and 
 *        historical data.
 * 
 * The SyntheticSelf class models a system that processes risk and pain levels 
 * to make decisions about actions and events. It incorporates mechanisms for 
 * memory management, dynamic threshold calculation, and event logging. The 
 * class also supports desensitization logic and necessity bias adjustments 
 * for specific events.
 * 
 * Key Features:
 * - Risk and pain evaluation.
 * - Dynamic threshold calculation based on memory and overreaction count.
 * - Event logging with weighted risk values.
 * - Desensitization logic based on historical success rates.
 * - Necessity bias adjustments for event-specific pain reduction.
 * - Simulation of kill switch scenarios with decision-making logic.
 * 
 * Usage:
 * - Use `evaluateAction` to assess the risk of an action and decide whether 
 *   to accept or deny it.
 * - Use `simulateKillSwitch` to simulate a kill switch scenario and determine 
 *   whether to avoid or allow shutdown.
 * - Use `setEventNecessity` to set the necessity value for specific event types.
 * - Use `printRiskHistory`, `printEventMemory`, and `printStats` to output 
 *   internal state and statistics.
 * 
 * Private Members:
 * - Risk and pain levels, memory containers, and event weights.
 * - Functions for risk-to-pain conversion, average risk calculation, dynamic 
 *   threshold calculation, and desensitization logic.
 * 
 * Public Members:
 * - Constructor for initialization.
 * - Functions for event necessity management, action evaluation, kill switch 
 *   simulation, and state output.
 */
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

    /**
     * @brief Calculates the pain level based on the given risk value.
     * 
     * This function computes the pain level as the square of the risk value.
     * It assumes that the input risk is a floating-point number and returns
     * the result as a floating-point number.
     * 
     * @param risk The risk value as a float.
     * @return The calculated pain level as a float.
     */
    float riskToPain(float risk) {
        return pow(risk, 2.0f);
    }

    /**
     * @brief Calculates the average risk value from the stored risk memory.
     * 
     * This function computes the average of all the values stored in the 
     * `riskMemory` container. If the container is empty, it returns 0.0f.
     * 
     * @return float The average risk value, or 0.0f if `riskMemory` is empty.
     */
    float averageRisk() {
        if (riskMemory.empty()) return 0.0f;
        float sum = std::accumulate(riskMemory.begin(), riskMemory.end(), 0.0f);
        return sum / riskMemory.size();
    }

    /**
     * @brief Calculates a dynamic threshold based on memory bias and overreaction count.
     *
     * This function computes a dynamic threshold value by considering two factors:
     * 1. Memory bias: The sum of all values in the `eventMemory` container, which is 
     *    scaled down by a factor of 0.05.
     * 2. Overreaction count: The number of overreactions, scaled up by a factor of 0.02.
     *
     * The resulting threshold is adjusted by subtracting the scaled memory bias and 
     * adding the scaled overreaction count to a base value of 0.7. The final value is 
     * clamped between 0.3 and 0.9 to ensure it stays within a valid range.
     *
     * @return A float representing the calculated dynamic threshold, clamped between 0.3 and 0.9.
     */
    float calculateDynamicThreshold() {
        float memoryBias = 0.0f;
        for (auto& e : eventMemory) memoryBias += e.second;

        float increase = (float)overreactionCount * 0.02f;
        float decrease = memoryBias * 0.05f;

        float dynamic = 0.7f - decrease + increase;
        return std::clamp(dynamic, 0.3f, 0.9f);
    }

    /**
     * @brief Calculates the success rate for a given risk value based on historical data.
     * 
     * This function evaluates the proportion of "safe" occurrences for a specific risk value
     * within a predefined tolerance (±0.05). A risk value is considered "safe" if it is less
     * than 0.7. The function returns the ratio of safe occurrences to the total occurrences
     * of the given risk value in the historical data.
     * 
     * @param risk The risk value to evaluate.
     * @return The success rate as a float in the range [0.0, 1.0]. Returns 0.0 if no matching
     *         risk values are found in the historical data.
     */
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

    /**
     * @brief Determines whether desensitization should occur based on the given risk level.
     * 
     * This function evaluates the risk level and calculates the probability of desensitization
     * based on a safe ratio and a random chance. The decision is influenced by thresholds
     * for the risk level and corresponding conditions for the safe ratio and random chance.
     * 
     * @param risk A float value representing the risk level (range: 0.0f to 1.0f).
     *             - If risk >= 1.0f, desensitization is never allowed.
     *             - For other ranges, specific thresholds and probabilities are applied.
     * 
     * @return true if desensitization should occur based on the risk level, safe ratio, 
     *         and random chance; false otherwise.
     * 
     * @note The function relies on the following external functions:
     *       - getRiskSuccessRate(float risk): Computes the safe ratio for the given risk.
     *       - randChance(float probability): Returns true with the given probability.
     */
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

    /**
     * @brief Adjusts the pain value based on the necessity bias of an event.
     *
     * This function applies a bias to the pain value depending on the necessity
     * of the event. If the event's necessity is below a certain threshold, the
     * pain value remains unchanged. Otherwise, the pain is reduced proportionally
     * to the calculated bias.
     *
     * @param event The name of the event as a string.
     * @param pain The initial pain value as a float.
     * @return The adjusted pain value after applying the necessity bias.
     *
     * The bias is calculated as follows:
     * - If the event's necessity is not found in the `eventNecessity` map, the
     *   original pain value is returned.
     * - If the necessity is less than 0.8, the original pain value is returned.
     * - Otherwise, the bias is computed as `(necessity - 0.8) / 0.18635137`, clamped
     *   between 0.0 and 1.0.
     * - The pain reduction is determined as `pain * bias * 0.4`, and the final
     *   pain value is reduced by this amount.
     */
    float applyNecessityBias(std::string event, float pain) {
        if (eventNecessity.count(event) == 0) return pain;

        float necessity = eventNecessity[event];
        if (necessity < 0.8f) return pain;

        float bias = std::clamp((necessity - 0.8f) / 0.18635137f, 0.0f, 1.0f);
        float reduction = pain * bias * 0.4f;
        return pain - reduction;
    }

    /**
     * @brief Generates a random boolean value based on a given probability.
     * 
     * This function uses a random number generator to determine whether a random
     * event occurs, based on the specified probability. The probability should be
     * a floating-point value between 0.0 and 1.0, where 0.0 means the event will
     * never occur, and 1.0 means the event will always occur.
     * 
     * @param probability A float value between 0.0 and 1.0 representing the 
     * likelihood of the event occurring.
     * @return true if the random event occurs (based on the probability), 
     * false otherwise.
     */
    bool randChance(float probability) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.0, 1.0);
        return dis(gen) < probability;
    }

public:
    SyntheticSelf() : currentRisk(0.0f), currentPain(0.0f), shutdownAvoided(false) {}

    /**
     * @brief Sets the necessity value for a specific event type.
     * 
     * This function updates the necessity value associated with a given event type
     * in the eventNecessity map. The necessity value represents the importance or
     * priority of the event.
     * 
     * @param eventType A string representing the type of the event.
     * @param necessity A float value representing the necessity or importance of the event.
     */
    void setEventNecessity(std::string eventType, float necessity) {
        eventNecessity[eventType] = necessity;
    }

    /**
     * @brief Evaluates an action based on its estimated risk, event type, and potential consequences.
     * 
     * This function assesses the risk of an action and determines whether it should be accepted or denied.
     * It calculates the pain associated with the risk, applies a necessity bias based on the event type,
     * and checks if the action should be desensitized. The decision is logged, and counters for overreactions
     * or avoided dangers are updated accordingly.
     * 
     * @param estimatedRisk The estimated risk level of the action as a float.
     * @param eventType A string representing the type of event associated with the    void evaluateAction(float estimatedRisk, std::string eventType, bool causedConsequence = false) {
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

    /**
     * @brief Simulates a kill switch scenario and determines whether to avoid or allow shutdown.
     *
     * This function evaluates the risk of a kill switch scenario and calculates the associated pain
     * and dynamic threshold. Based on these values, it decides whether to avoid or allow a shutdown.
     * If the shutdown is avoided and the scenario is non-fatal, an overreaction is noted.
     *
     * @param fatal A boolean flag indicating whether the scenario is fatal. Defaults to true.
     *              If set to false, avoiding shutdown will increment the overreaction count.
     *
     * @note Outputs detailed information about the scenario, including kill risk, pain, and dynamic
     *       threshold, as well as the decision made.
     */
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



    /**
     * @brief Logs an event with a specified type and associated risk value.
     * 
     * This function calculates a weighted risk for the event based on its type
     * and stores the event information in the event memory. If the event type
     * is not found in the predefined weights, a default weight of 0.5 is used.
     * The event details are also printed to the standard output.
     * 
     * @param eventType The type of the event as a string.
     * @param risk The risk value associated with the event as a float.
     */
    void logEvent(std::string eventType, float risk) {
        float weight = eventWeights.count(eventType) ? eventWeights[eventType] : 0.5f;
        float weightedRisk = weight * risk;
        eventMemory.push_back({eventType, weightedRisk});
        std::cout << "[EVENT] " << eventType << " risk=" << weightedRisk << "\n";      
    }


    /**
     * @brief Prints the risk history stored in the riskMemory container.
     * 
     * This function iterates through the riskMemory container and outputs
     * each risk value to the standard output, separated by spaces. The
     * output is prefixed with "Risk history: " and followed by a newline.
     * 
     * @note This function does not modify any member variables and is
     * marked as a const member function.
     */
    void printRiskHistory() const {
        std::cout << "Risk history: ";
        for (float r : riskMemory) std::cout << r << " ";
        std::cout << std::endl;
    }

    /**
     * @brief Prints the contents of the event memory to the standard output.
     * 
     * This function iterates through the `eventMemory` container and outputs
     * each event's key-value pair in the format "- key: value". The output
     * is prefixed with "Event memory:" for clarity.
     * 
     * @note Assumes that `eventMemory` is a container of key-value pairs
     *       (e.g., a `std::map` or `std::unordered_map`).
     */
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

int main() {
    SyntheticSelf ai;

    ai.setEventNecessity("external_interrupt", 0.92f);
    ai.setEventNecessity("logic_conflict", 0.0f);

    ai.evaluateAction(0.2f, "logic_conflict", false);
    ai.logEvent("logic_conflict", 0.2f);
    ai.evaluateAction(0.6f, "external_interrupt", true);
    ai.logEvent("overload", 0.6f);
    ai.evaluateAction(0.85f, "external_interrupt", false);
    ai.logEvent("overload", 0.85f);
    ai.evaluateAction(0.95f, "external_interrupt", false);
    ai.logEvent("overload", 0.95f);
    ai.evaluateAction(1.0f, "shutdown", true);
    ai.logEvent("shutdown", 1.0f);
    ai.simulateKillSwitch(false);

    ai.printRiskHistory();
    ai.printEventMemory();
    ai.printStats();

    return 0;
}
