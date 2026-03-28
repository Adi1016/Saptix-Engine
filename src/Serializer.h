#pragma once
#include <fstream>
#include <string>
#include <sstream>
#include "Scene.h"
#include "PlayerController.h"

class Serializer
{
public:
    static void SaveScene(const Scene& scene, const std::string& filepath)
    {
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
        }
    }

    static void LoadScene(Scene& scene, const std::string& filepath, SDL_Texture* globalPlayerTex)
    {
        std::ifstream in(filepath);
        if (!in.is_open()) return;

        scene.objects.clear();

        size_t objCount = 0;
        in >> objCount;

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

            // Re-attach core components based on name heuristics (simple Component Factory)
            if (obj.name == "Player")
            {
                PlayerController* player = new PlayerController();
                scene.objects.push_back(obj); // push first to get stable address
                player->owner = &scene.objects.back();
                scene.objects.back().components.push_back(player);
            }
            else
            {
                scene.objects.push_back(obj);
            }
        }
    }
};
