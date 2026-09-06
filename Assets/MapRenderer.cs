using NUnit.Framework;
using System.Collections.Generic;
using System.Net;
using UnityEngine;

public class MapRenderer : MonoBehaviour
{
    public static MapRenderer Instance;

    private Sprite tileSprite;
    private GameObject[] tileObjects;
    private int[] tileTypes;

    void Awake()
    {
        Instance = this;
        tileSprite = CreateSquareSprite();
    }
    
    Sprite CreateSquareSprite()
    {
        Texture2D tex = new Texture2D(1, 1);
        tex.SetPixel(0, 0, Color.white);
        tex.Apply();
        return Sprite.Create(tex, new Rect(0, 0, 1, 1), new Vector2(0f, 0f), 1f);
    }

    private int mapWidth;
    private int mapHeight;

    public void RenderMap(int width, int height, int[] tiles)
    {
        mapWidth = width;
        mapHeight = height;
        tileTypes = tiles;

        if (tileObjects != null)
        {
            foreach(var obj in tileObjects)
            {
                if (obj != null) Destroy(obj);
            }
        }
        tileObjects = new GameObject[width * height];

        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                int index = y * width + x;
                int type = tiles[index];

                GameObject tileObj = new GameObject($"Tile_{x}_{y}");
                tileObj.transform.position = new Vector3(x, y, 0);

                SpriteRenderer sr = tileObj.AddComponent<SpriteRenderer>();
                sr.sprite = tileSprite;
                sr.color = GetColorForTile(type);

                tileObjects[index] = tileObj;
            }
        }
    }

    public bool IsWalkable(int x, int y)
    {
        if (x < 0 || x >= mapWidth || y < 0 || y >= mapHeight) return false;
        return tileTypes[y * mapWidth + x] == 0;
    }

    public void ClearTiles(List<(int, int)> tilesToClear)
    {
        foreach (var (x, y) in tilesToClear)
        {
            int index = y * mapWidth + x;
            if (index < 0 || index >= tileObjects.Length || tileObjects[index] == null) continue;

            tileObjects[index].GetComponent<SpriteRenderer>().color = Color.white;
            tileTypes[index] = 0;
        }
    }

    Color GetColorForTile(int type)
    {
        switch (type)
        {
            case 1: return Color.gray;
            case 2: return new Color(0.6f, 0.4f, 0.2f);
            default: return Color.white;
        }
    }
}
