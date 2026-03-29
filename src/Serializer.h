#pragma once
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include "Scene.h"
#include "PlayerController.h"

class Serializer
{
public:
    static std::vector<std::string> GetRecentProjects()
    {
        std::vector<std::string> projects;
        std::ifstream in("saptix_config.txt");
        if (!in.is_open()) return projects;

        std::string line;
        while (std::getline(in, line))
        {
            if (!line.empty()) projects.push_back(line);
        }
        return projects;
    }

    static void AddRecentProject(const std::string& path)
    {
        std::vector<std::string> projects = GetRecentProjects();
        // Remove if config already has it (so we can move to top)
        projects.erase(std::remove(projects.begin(), projects.end(), path), projects.end());
        // Insert at the front (most recent)
        projects.insert(projects.begin(), path);

        std::ofstream out("saptix_config.txt");
        for (const std::string& p : projects)
        {
            out << p << "\n";
        }
    }

    static void SaveScene(const Scene& scene, const std::string& filepath)
    {
        AddRecentProject(filepath); // Auto record history globally

        std::ofstream out(filepath);
        if (!out.is_open()) return;

        out << scene.objects.size() << "\n";

        for (const auto& obj : scene.objects)
        {
            out << "[OBJECT]\n";
            // Replace spaces with underscores for safe string block reading
            std::string safeName = obj.name;
            for (char& c : safeName) if (c == ' ') c = '_';
            
            out << safeName << "\n";
            out << obj.position.x << " " << obj.position.y << "\n";
            out << obj.size.x << " " << obj.size.y << "\n";
            out << obj.velocity.x << " " << obj.velocity.y << "\n";
            out << obj.flipHorizontal << " " << obj.isGrounded << " " << (obj.texture ? 1 : 0) << "\n";

            auto WriteVec = [&](const std::vector<SDL_FRect>& v) {
                out << v.size() << "\n";
                for (const auto& r : v) out << r.x << " " << r.y << " " << r.w << " " << r.h << "\n";
            };

            WriteVec(obj.framesIdle);
            WriteVec(obj.framesWalk);
            WriteVec(obj.framesJump);

            out << obj.components.size() << "\n";
            for (auto* comp : obj.components)
            {
                out << comp->GetName() << "\n";
            }
        }
    }

    static void LoadScene(Scene& scene, const std::string& filepath, SDL_Texture* globalPlayerTex)
    {
        std::ifstream in(filepath);
        if (!in.is_open()) return;

        scene.objects.clear();

        size_t objCount = 0;
        in >> objCount;
        scene.objects.reserve(objCount + 100); // Reserve extra to avoid reallocation padding during active editing

        for (size_t i = 0; i < objCount; i++)
        {
            std::string header;
            in >> header; // [OBJECT]
            if (header != "[OBJECT]") break;

            GameObject obj;
            std::string rawName;
            in >> std::ws >> rawName;
            for (char& c : rawName) if (c == '_') c = ' ';
            obj.name = rawName;

            in >> obj.position.x >> obj.position.y;
            in >> obj.size.x >> obj.size.y;
            in >> obj.velocity.x >> obj.velocity.y;
            
            bool flip, grounded, hasTex;
            in >> flip >> grounded >> hasTex;
            obj.flipHorizontal = flip;
            obj.isGrounded = grounded;
            if (hasTex) obj.texture = globalPlayerTex;

            auto ReadVec = [&](std::vector<SDL_FRect>& v) {
                size_t count = 0;
                in >> count;
                for (size_t c = 0; c < count; c++)
                {
                    SDL_FRect r;
                    in >> r.x >> r.y >> r.w >> r.h;
                    v.push_back(r);
                }
            };

            ReadVec(obj.framesIdle);
            ReadVec(obj.framesWalk);
            ReadVec(obj.framesJump);

            scene.objects.push_back(obj);

            // Re-attach core components dynamically (with legacy support rollback)
            auto pos = in.tellg();
            std::string peekToken;
            if (in >> peekToken)
            {
                if (peekToken == "[OBJECT]")
                {
                    in.seekg(pos); // Rollback, this is a legacy object with no components
                }
                else
                {
                    size_t compCount = 0;
                    try { compCount = std::stoull(peekToken); } catch(...) {}
                    for (size_t c = 0; c < compCount; c++)
                    {
                        std::string compName;
                        in >> std::ws >> compName;
                        if (compName == "PlayerController")
                        {
                            PlayerController* pc = new PlayerController();
                            pc->owner = &scene.objects.back();
                            scene.objects.back().components.push_back(pc);
                        }
                        else if (compName == "HealthComponent")
                        {
                            HealthComponent* hc = new HealthComponent();
                            hc->owner = &scene.objects.back();
                            scene.objects.back().components.push_back(hc);
                        }
                        else if (compName == "CombatComponent")
                        {
                            CombatComponent* cc = new CombatComponent();
                            cc->owner = &scene.objects.back();
                            scene.objects.back().components.push_back(cc);
                        }
                    }
                }
            }
        }
    }
};
