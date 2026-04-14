#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>
#include <unordered_map>

using std::string;
using std::vector;
using std::array;

// --------------------- Enums & Constants ---------------------
enum class Attr : int {
    Vitality = 0,
    Strength,
    Agility,
    Intelligence,
    Will,
    Perception,
    Resonance,
    COUNT
};

static constexpr int ATTR_N = static_cast<int>(Attr::COUNT);

static const char* attrName(Attr a) {
    switch (a) {
        case Attr::Vitality:     return "Vitality";
        case Attr::Strength:     return "Strength";
        case Attr::Agility:      return "Agility";
        case Attr::Intelligence: return "Intelligence";
        case Attr::Will:         return "Will";
        case Attr::Perception:   return "Perception";
        case Attr::Resonance:    return "Resonance";
        default:                 return "Unknown";
    }
}

static const char* titleName(int t) {
    static const char* names[] = { "Novice", "Apprentice", "Journeyman", "Adept", "Expert", "Master", "Grandmaster", "Transcendent" };
    if (t < 0) t = 0;
    if (t > 7) t = 7;
    return names[t];
}

// --------------------- Data Classes ---------------------

class Skill {
private:
    string name;
    int level;
    int Ldis;
    int skilltitle;
    bool eligibleTopK;

    void recomputeLevel() {
        if (skilltitle < 0) skilltitle = 0;
        if (skilltitle > 7) skilltitle = 7;
        if (Ldis < 1) Ldis = 1;
        if (Ldis > 10) Ldis = 10;
        level = 10 * skilltitle + Ldis;
    }

public:
    Skill() : name(""), level(0), Ldis(1), skilltitle(0), eligibleTopK(true) { recomputeLevel(); }
    Skill(string n, int title, int displayLvl, bool eligible) 
        : name(n), Ldis(displayLvl), skilltitle(title), eligibleTopK(eligible) {
        recomputeLevel();
    }

    string getName() const { return name; }
    int getLevel() const { return level; }
    int getTitle() const { return skilltitle; }
    int getDisplayLevel() const { return Ldis; }
    bool isEligible() const { return eligibleTopK; }

    void setTitle(int title) { skilltitle = title; recomputeLevel(); }
    void setDisplayLevel(int displayLvl) { Ldis = displayLvl; recomputeLevel(); }
};

class Job {
private:
    string name;
    int level;
    bool archived;

public:
    Job(string n, int lvl, bool arch) : name(n), level(lvl), archived(arch) {}

    string getName() const { return name; }
    int getLevel() const { return level; }
    bool isArchived() const { return archived; }

    void setLevel(int lvl) { level = lvl; }
};

struct CurveParams {
    double a = 0.0, b = 0.0, p = 1.0;
    double A = 0.0, B = 0.0, q = 1.0;
};

class Race {
private:
    string name;
    array<double, ATTR_N> deltaMin{};
    array<double, ATTR_N> deltaMax{};
    array<double, ATTR_N> epsMin{};
    array<double, ATTR_N> epsMax{};

public:
    Race(string n) : name(n) {}

    void setModifiers(Attr a, double dMin, double dMax, double eMin, double eMax) {
        int i = static_cast<int>(a);
        deltaMin[i] = dMin; deltaMax[i] = dMax;
        epsMin[i] = eMin; epsMax[i] = eMax;
    }

    string getName() const { return name; }
    double getDeltaMin(int i) const { return deltaMin[i]; }
    double getDeltaMax(int i) const { return deltaMax[i]; }
    double getEpsMin(int i) const { return epsMin[i]; }
    double getEpsMax(int i) const { return epsMax[i]; }
};

// --------------------- System Engines ---------------------

class AttributeSystem {
private:
    double gamma = 0.04;
    array<CurveParams, ATTR_N> curves;

    double c_max(int L) const {
        if (L <= 1) return 1.0;
        return 1.0 / (1.0 + gamma * (static_cast<double>(L) - 1.0));
    }

    double h(int L) const {
        return std::log(1.0 + static_cast<double>(L));
    }

public:
    AttributeSystem() {
        auto setAll = [&](Attr a, double min_a, double min_b, double min_p, double max_A, double max_B, double max_q) {
            int i = static_cast<int>(a);
            curves[i].a = min_a; curves[i].b = min_b; curves[i].p = min_p;
            curves[i].A = max_A; curves[i].B = max_B; curves[i].q = max_q;
        };
        setAll(Attr::Vitality, 8, 1.15, 0.92, 14, 1.85, 1.05);
        setAll(Attr::Strength, 6, 1.10, 0.92, 12, 1.70, 1.05);
        setAll(Attr::Agility, 6, 1.10, 0.92, 12, 1.70, 1.05);
        setAll(Attr::Intelligence, 6, 1.05, 0.92, 12, 1.60, 1.05);
        setAll(Attr::Will, 6, 1.00, 0.92, 12, 1.50, 1.05);
        setAll(Attr::Perception, 6, 1.00, 0.92, 12, 1.50, 1.05);
        setAll(Attr::Resonance, 6, 1.00, 0.92, 12, 1.50, 1.05);
    }

    void computeWindowAtL(int L, const Race& r, array<int, ATTR_N>& outMin, array<int, ATTR_N>& outMax) const {
        if (L < 0) L = 0;
        for (int i = 0; i < ATTR_N; ++i) {
            const auto& p = curves[i];
            double mA = p.a + p.b * std::pow(static_cast<double>(L), p.p);
            double MA = p.A + p.B * std::pow(static_cast<double>(L), p.q);
            double rawMin = mA + r.getDeltaMin(i) + r.getEpsMin(i) * h(L);
            double rawMax = MA + r.getDeltaMax(i) * c_max(L) + r.getEpsMax(i) * h(L);

            int minI = static_cast<int>(std::floor(rawMin));
            int maxI = static_cast<int>(std::ceil(rawMax));
            if (maxI < minI + 1) maxI = minI + 1;

            outMin[i] = minI;
            outMax[i] = maxI;
        }
    }
};

class LevelSystem {
private:
    double wJ = 0.20;
    double ws = 0.35;

public:
    int computeK(int TL_prev) const {
        if (TL_prev < 20) return 6;
        if (TL_prev < 60) return 8;
        long long threshold = 60;
        int K = 10;
        while (static_cast<long long>(TL_prev) >= threshold * 2) {
            threshold *= 2;
            K += 2;
        }
        return K;
    }

    double getWJ() const { return wJ; }
    double getWS() const { return ws; }
};

// --------------------- Main Character Class ---------------------

class Character {
private:
    string name;
    int raceIndex;
    vector<Skill> skills;
    vector<Job> jobs;
    std::unordered_map<string, int> skillIndex;
    std::unordered_map<string, int> jobIndex;

    double TL_raw = 0.0;
    int TL = 0;
    int TL_prev = 0;
    int K_used = 6;
    
    bool attrsInitialized = false;
    array<int, ATTR_N> E{};    
    array<int, ATTR_N> Min{};
    array<int, ATTR_N> Max{};  

public:
    Character(string n, int rIndex, int startLevel) : name(n), raceIndex(rIndex), TL_prev(startLevel) {}

    // Methods for Skills and Jobs
    void addSkill(const Skill& s) {
        skills.push_back(s);
        skillIndex[s.getName()] = static_cast<int>(skills.size()) - 1;
    }

    void addJob(const Job& j) {
        jobs.push_back(j);
        jobIndex[j.getName()] = static_cast<int>(jobs.size()) - 1;
    }

    int findSkillIndex(const string& skillName) const {
        auto it = skillIndex.find(skillName);
        return (it == skillIndex.end()) ? -1 : it->second;
    }

    int findJobIndex(const string& jobName) const {
        auto it = jobIndex.find(jobName);
        return (it == jobIndex.end()) ? -1 : it->second;
    }

    Skill& getSkill(int index) { return skills[index]; }
    Job& getJob(int index) { return jobs[index]; }
    const vector<Skill>& getAllSkills() const { return skills; }
    const vector<Job>& getAllJobs() const { return jobs; }

    // Core Logic Systems
    void updateLeveling(const LevelSystem& sys) {
        K_used = sys.computeK(TL_prev);
        
        int jobScore = 0;
        for (const auto& j : jobs) jobScore += j.getLevel();

        double topKSum = 0.0;
        std::vector<int> scores;
        for (const auto& s : skills) {
            if (s.isEligible()) scores.push_back(s.getLevel());
        }

        int n = static_cast<int>(scores.size());
        if (n > 0) {
            const int m = std::min(K_used + 2, n);
            if (m < n) {
                std::nth_element(scores.begin(), scores.begin() + (m - 1), scores.end(), std::greater<int>());
            }
            std::sort(scores.begin(), scores.begin() + m, std::greater<int>());

            for (int i = 0; i < std::min(K_used, n); ++i) topKSum += scores[i];
            if (n > K_used) topKSum += 0.5 * scores[K_used];
            if (n > K_used + 1) topKSum += 0.5 * scores[K_used + 1];
        }

        TL_raw = (sys.getWJ() * jobScore) + (sys.getWS() * topKSum);
        TL = static_cast<int>(std::floor(TL_raw));
        TL_prev = TL;
    }

    void syncAttributes(const AttributeSystem& attrSys, const Race& r) {
        array<int, ATTR_N> newMin{}, newMax{};
        attrSys.computeWindowAtL(TL, r, newMin, newMax);

        for (int i = 0; i < ATTR_N; ++i) {
            if (attrsInitialized) {
                newMin[i] = std::max(newMin[i], Min[i]);
                newMax[i] = std::max(newMax[i], Max[i]);
            }
            newMax[i] = std::max(newMax[i], newMin[i] + 1);
            
            if (!attrsInitialized) {
                E[i] = newMin[i];
            } else {
                E[i] = std::clamp(E[i], newMin[i], newMax[i]);
            }
        }
        Min = newMin;
        Max = newMax;
        attrsInitialized = true;
    }

    void trainAttribute(Attr a, int delta) {
        int i = static_cast<int>(a);
        long long v = static_cast<long long>(E[i]) + static_cast<long long>(delta);
        if (v > Max[i]) v = Max[i];
        E[i] = static_cast<int>(v);
    }

    // Derived Stats
    int getHealth() const { return 10 * E[static_cast<int>(Attr::Vitality)] + 2 * E[static_cast<int>(Attr::Will)]; }
    int getStamina() const { return 6 * E[static_cast<int>(Attr::Vitality)] + 4 * E[static_cast<int>(Attr::Agility)]; }
    int getMana() const { return 8 * E[static_cast<int>(Attr::Intelligence)] + 4 * E[static_cast<int>(Attr::Resonance)]; }

    // Getters / Setters
    string getName() const { return name; }
    int getRaceIndex() const { return raceIndex; }
    void setRaceIndex(int idx) { raceIndex = idx; }
    int getTL() const { return TL; }
    double getTLRaw() const { return TL_raw; }
    int getKUsed() const { return K_used; }
    int getE(int i) const { return E[i]; }
    int getMin(int i) const { return Min[i]; }
    int getMax(int i) const { return Max[i]; }
};

// --------------------- Input Helpers & Setup ---------------------

static void clearLine() {
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

static bool readYesNo(const std::string& prompt) {
    while (true) {
        std::cout << prompt << " (y/n): ";
        std::string s;
        if (!std::getline(std::cin, s)) std::exit(0);
        if (s == "y" || s == "Y") return true;
        if (s == "n" || s == "N") return false;
        std::cout << "Please type y or n.\n";
    }
}

static int readInt(const std::string& prompt, int minV = std::numeric_limits<int>::min(), int maxV = std::numeric_limits<int>::max()) {
    while (true) {
        std::cout << prompt;
        long long v;
        if (std::cin >> v) {
            clearLine();
            if (v < minV || v > maxV) {
                std::cout << "Out of range.\n";
                continue;
            }
            return static_cast<int>(v);
        }
        if (std::cin.eof()) std::exit(0);
        std::cin.clear();
        clearLine();
        std::cout << "Invalid number.\n";
    }
}

static std::vector<Race> buildDefaultRaces() {
    std::vector<Race> races;
    races.push_back(Race("Human (baseline)"));
    
    Race beast("Beast-kin (placeholder)");
    beast.setModifiers(Attr::Agility, 2.0, 4.0, 0.0, 0.0);
    beast.setModifiers(Attr::Perception, 1.0, 2.0, 0.0, 0.0);
    races.push_back(beast);

    Race demon("Demon-kin (placeholder)");
    demon.setModifiers(Attr::Resonance, 2.0, 5.0, 0.0, 0.0);
    demon.setModifiers(Attr::Will, 1.0, 2.0, 0.0, 0.0);
    races.push_back(demon);

    return races;
}

static int chooseRaceIndex(const std::vector<Race>& races) {
    std::cout << "\nRaces:\n";
    for (int i = 0; i < static_cast<int>(races.size()); ++i) {
        std::cout << "  " << i << ") " << races[i].getName() << "\n";
    }
    return readInt("Choose race index: ", 0, static_cast<int>(races.size()) - 1);
}

static void printStatus(const Character& c, const Race& r) {
    std::cout << "\nSTATUS: " << c.getName() << "\n";
    std::cout << "Race: " << r.getName() << "\n";
    std::cout << "TL: " << c.getTL() << "   (TL_raw: " << c.getTLRaw() << ")   K used: " << c.getKUsed() << "\n";
    
    std::cout << "\nAttributes (E in [Min, Max]):\n";
    for (int i = 0; i < ATTR_N; ++i) {
        Attr a = static_cast<Attr>(i);
        std::cout << "  - " << attrName(a) << ": " << c.getE(i) << "  [" << c.getMin(i) << ", " << c.getMax(i) << "]\n";
    }

    std::cout << "\nPools:\n";
    std::cout << "  Health:  " << c.getHealth() << "\n";
    std::cout << "  Stamina: " << c.getStamina() << "\n";
    std::cout << "  Mana:    " << c.getMana() << "\n";

    std::cout << "\nJobs:\n";
    if (c.getAllJobs().empty()) std::cout << "  (none)\n";
    for (const auto& j : c.getAllJobs()) {
        std::cout << "  - " << j.getName() << " Lv " << j.getLevel() << (j.isArchived() ? " (archived)\n" : " (active)\n");
    }

    std::cout << "\nSkills:\n";
    if (c.getAllSkills().empty()) std::cout << "  (none)\n";
    for (const auto& s : c.getAllSkills()) {
        std::cout << "  - " << s.getName() << "  " << titleName(s.getTitle()) << " " << s.getDisplayLevel()
                  << "  [S=" << s.getLevel() << "]" << (s.isEligible() ? "\n" : " (excluded)\n");
    }
    std::cout << "\n";
}

int main() {
    LevelSystem sys;
    AttributeSystem attrSys;
    std::vector<Race> races = buildDefaultRaces();

    std::cout << "OOP System_v3 TL simulator\n";
    std::cout << "-------------------------------------\n";

    string charName;
    std::cout << "Character name: ";
    if (!std::getline(std::cin, charName)) std::exit(0);
    if (charName.empty()) charName = "Unnamed";

    int rIdx = chooseRaceIndex(races);
    int startLevel = readInt("Starting level (usually 0 for new): ", 0, 9999999);
    
    Character c(charName, rIdx, startLevel);

    c.updateLeveling(sys);
    c.syncAttributes(attrSys, races[c.getRaceIndex()]);
    printStatus(c, races[c.getRaceIndex()]);

    while (true) {
        std::cout << "Menu:\n  1) Show status\n  2) Run TL update tick\n  3) Set a skill displayed level/title\n"
                  << "  4) Set a job level\n  5) Add a new skill\n  6) Add a new job\n  7) Train an attribute (+Delta)\n"
                  << "  8) Change race\n  0) Exit\n";

        int choice = readInt("Choice: ", 0, 8);
        if (choice == 0) break;

        if (choice == 1) {
            printStatus(c, races[c.getRaceIndex()]);
        } else if (choice == 2) {
            c.updateLeveling(sys);
            c.syncAttributes(attrSys, races[c.getRaceIndex()]);
            std::cout << "\nUpdate complete.\n";
            printStatus(c, races[c.getRaceIndex()]);
        } else if (choice == 3) {
            std::cout << "Skill name (exact): ";
            string name;
            std::getline(std::cin, name);
            int idx = c.findSkillIndex(name);
            if (idx < 0) { std::cout << "Skill not found.\n\n"; continue; }
            
            if (readYesNo("Change title band")) c.getSkill(idx).setTitle(readInt("New skill title (0-7): ", 0, 7));
            c.getSkill(idx).setDisplayLevel(readInt("New displayed level (1-10): ", 1, 10));
            std::cout << "Updated.\n\n";
        } else if (choice == 4) {
            std::cout << "Job name: ";
            string name;
            std::getline(std::cin, name);
            int idx = c.findJobIndex(name);
            if (idx < 0) { std::cout << "Job not found.\n\n"; continue; }
            
            c.getJob(idx).setLevel(readInt("New job level (0-100): ", 0, 100));
            std::cout << "Updated.\n\n";
        } else if (choice == 5) {
            std::cout << "New skill name: ";
            string name;
            std::getline(std::cin, name);
            if (c.findSkillIndex(name) >= 0) { std::cout << "Skill already exists.\n\n"; continue; }
            
            int title = readInt("Skill title (0-7): ", 0, 7);
            int dLvl = readInt("Displayed level inside title (1-10): ", 1, 10);
            bool eligible = !readYesNo("Exclude from TopK");
            
            c.addSkill(Skill(name, title, dLvl, eligible));
            std::cout << "Added.\n\n";
        } else if (choice == 6) {
            std::cout << "New job name: ";
            string name;
            std::getline(std::cin, name);
            if (c.findJobIndex(name) >= 0) { std::cout << "Job already exists.\n\n"; continue; }
            
            int lvl = readInt("Job level (0-100): ", 0, 100);
            bool archived = readYesNo("Archived");
            
            c.addJob(Job(name, lvl, archived));
            std::cout << "Added.\n\n";
        } else if (choice == 7) {
            int ai = readInt("Attribute index (0-6): ", 0, ATTR_N - 1);
            int delta = readInt("Delta (training gain): ", 0, 100);
            c.trainAttribute(static_cast<Attr>(ai), delta);
            std::cout << "Trained.\n\n";
        } else if (choice == 8) {
            c.setRaceIndex(chooseRaceIndex(races));
            c.syncAttributes(attrSys, races[c.getRaceIndex()]);
            std::cout << "Race changed.\n\n";
        }
    }
    return 0;
}