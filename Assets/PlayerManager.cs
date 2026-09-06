using System.Collections.Generic;
using UnityEngine;

public class PlayerManager : MonoBehaviour 
{
    public static PlayerManager Instance;

    private Dictionary<int, GameObject> players = new Dictionary<int, GameObject>();
    private Sprite playerSprite;

    void Awake()
    {
        Instance = this;
        playerSprite = CreateCircleSprite();
    }

    Sprite CreateCircleSprite()
    {
        Texture2D tex = new Texture2D(1, 1);
        tex.SetPixel(0, 0, Color.white);
        tex.Apply();

        return Sprite.Create(tex, new Rect(0, 0, 1, 1), new Vector2(0.5f, 0.5f), 1f);
    }

    public GameObject SpawnPlayer(int userId, float x, float y, bool isMe)
    {
        GameObject obj = new GameObject(isMe ? "MyPlayer" : $"Player_{userId}");
        obj.transform.position = new Vector3(x, y, -1f);
        SpriteRenderer sr = obj.AddComponent<SpriteRenderer>();
        sr.sprite = playerSprite;
        sr.color = isMe ? Color.blue : Color.red;

        players[userId] = obj;
        return obj;
    }

    public void UpdatePlayerPosition(int userId, float x, float y)
    {
        if(!players.ContainsKey(userId))
        {
            SpawnPlayer(userId, x, y, false);
            return;
        }
        players[userId].transform.position = new Vector3(x, y, -1f);
    }

    public void SetPlayerDead(int userId)
    {
        if (players.TryGetValue(userId, out GameObject obj))
        {
            obj.SetActive(false);
        }
    }
}
