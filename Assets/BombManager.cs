using System.Collections.Generic;
using UnityEngine;

public class BombManager : MonoBehaviour
{
    public static BombManager Instance;

    private Dictionary<(int, int), GameObject> bombs = new Dictionary<(int, int), GameObject>();
    private Sprite bombSprite;

    void Awake()
    {
        Instance = this;
        bombSprite = CreateSquareSprite();
    }
    Sprite CreateSquareSprite()
    {
        Texture2D tex = new Texture2D(1, 1);
        tex.SetPixel(0, 0, Color.black);
        tex.Apply();
        return Sprite.Create(tex, new Rect(0, 0, 1, 1), new Vector2(0.5f, 0.5f), 1f);
    }

    public void SpawnBomb(float x, float y)
    {
        int tileX = Mathf.FloorToInt(x);
        int tileY = Mathf.FloorToInt(y);

        GameObject obj = new GameObject($"Bomb_{tileX}_{tileY}");
        obj.transform.position = new Vector3(x, y, -0.5f);

        SpriteRenderer sr = obj.AddComponent<SpriteRenderer>();
        sr.sprite = bombSprite;
        sr.color = Color.black;

        bombs[(tileX, tileY)] = obj;
    }

    public void RemoveBomb(int tileX, int tileY)
    {
        if (bombs.TryGetValue((tileX, tileY), out GameObject obj))
        {
            Destroy(obj);
            bombs.Remove((tileX, tileY));
        }
    }
}
