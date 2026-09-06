#pragma once
#include <vector>
#include <cmath>

constexpr int MAP_WIDTH = 15;
constexpr int MAP_HEIGHT = 15;

enum class TileType {
    EMPTY,
    WALL,       // 파괴 불가, 고정
    BREAKABLE   // 폭탄으로 파괴 가능
};

class Map {
public:
    Map(int width, int height)
        : width(width), height(height), tiles(width* height, TileType::EMPTY)
    {
        generateLayout();
    }

    TileType getTile(int tileX, int tileY) const {
        if (tileX < 0 || tileX >= width || tileY < 0 || tileY >= height) {
            return TileType::WALL;
        }
        return tiles[tileY * width + tileX];
    }

    bool destroyBlock(int tileX, int tileY) {
        if (tileX < 0 || tileX >= width || tileY < 0 || tileY >= height) {
            return false;
        }
        int index = tileY * width + tileX;
        if (tiles[index] == TileType::BREAKABLE) {
            tiles[index] = TileType::EMPTY;
            return true;
        }
        return false;
    }

    static void worldToTile(float worldX, float worldY, int& outTileX, int& outTileY) {
        outTileX = (int)std::floor(worldX);
        outTileY = (int)std::floor(worldY);
    }

    int getWidth() const { return width;  }
    int getHeight() const { return height; }
    const std::vector<TileType>& getTiles() const { return tiles; }

private:
    int width;
    int height;
    std::vector<TileType> tiles;

    void generateLayout() {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (x == 0 || y == 0 || x == width - 1 || y == height - 1) {
                    tiles[y * width + x] = TileType::WALL;
                }
                else {
                    tiles[y * width + x] = TileType::BREAKABLE;
                }
            }
        }

        int corners[4][2] = {
            {1, 1},
            {width - 2, 1},
            {1, height - 2},
            {width - 2, height - 2}
        };

        for (auto& c : corners) {
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    int x = c[0] + dx;
                    int y = c[1] + dy;
                    if (x > 0 && x < width - 1 && y > 0 && y < height - 1) {
                        tiles[y * width + x] = TileType::EMPTY;
                    }
                }
            }
        }
    }
};