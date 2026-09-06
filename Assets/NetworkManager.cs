using System;
using System.Collections.Generic;
using System.Net.Sockets;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading;
using Unity.VisualScripting;
using UnityEngine;

public enum PacketType : ushort
{
    MOVE = 1, BOMBPLANT = 2, BOOM = 3, PLAYERDIE = 4, START = 5, END = 6,
    JOIN = 7, NOTIFY = 8, CREATEROOM = 9, LISTROOM = 10, STARTGAME = 11,
    MAPINFO = 12, ASSIGNID = 13
}

public struct ReceivedPacket
{
    public PacketType type;
    public byte[] data;
}
public class NetworkManager : MonoBehaviour
{
    public static NetworkManager Instance;

    private int myId;
    private int myRoomId;
    private TcpClient client;
    private NetworkStream stream;
    private Thread receiveThread;
    private bool running = false;

    private byte[] recvBuffer = new byte[4096];
    private int recvSize = 0;

    private Queue<ReceivedPacket> packetQueue = new Queue<ReceivedPacket>();
    private object queueLock = new object();
    private GameObject myPlayerObject;
    private float myX, myY;

    void Awake()
    {
        Instance = this;
        DontDestroyOnLoad(gameObject);
        Application.runInBackground = true;
    }
    void Start()
    {
        Connect("127.0.0.1", 8080);
    }

    public void Connect(string ip, int port)
    {
        client = new TcpClient();
        client.Connect(ip, port);
        stream = client.GetStream();
        running = true;

        receiveThread = new Thread(ReceiveLoop);
        receiveThread.IsBackground = true;
        receiveThread.Start();
    }

    public void SendStartGame()
    {
        byte[] payload = BitConverter.GetBytes(myRoomId);
        SendPacket(PacketType.STARTGAME, payload);
    }
    public void ReceiveLoop()
    {
        byte[] tempBuffer = new byte[1024];
        while (running)
        {
            try
            {
                int bytesRead = stream.Read(tempBuffer, 0, tempBuffer.Length);
                if (bytesRead <= 0) break;

                Array.Copy(tempBuffer, 0, recvBuffer, recvSize, bytesRead);
                recvSize += bytesRead;

                const int HEADER_SIZE = 4;
                while (recvSize >= HEADER_SIZE)
                {
                    ushort type = BitConverter.ToUInt16(recvBuffer, 0);
                    ushort size = BitConverter.ToUInt16(recvBuffer, 2);

                    if (recvSize < size) break;

                    byte[] payload = new byte[size - HEADER_SIZE];
                    Array.Copy(recvBuffer, HEADER_SIZE, payload, 0, payload.Length);

                    lock (queueLock)
                    {
                        packetQueue.Enqueue(new ReceivedPacket { type = (PacketType)type, data = payload });
                    }

                    int remaining = recvSize - size;
                    Array.Copy(recvBuffer, size, recvBuffer, 0, remaining);
                    recvSize = remaining;
                }
            }
            catch (Exception e)
            {
                Debug.Log("Receive error: " + e.Message);
                break;
            }
        }
    }
    void Update()
    {
        lock (queueLock)
        {
            while (packetQueue.Count > 0)
            {
                HandlePacket(packetQueue.Dequeue());
            }
        }
        
        if (Input.GetKeyDown(KeyCode.Space))
        {
            SendStartGame();
        }

        if (Input.GetKeyDown(KeyCode.F))
        {
            SendBombPlant();
        }

        if (Input.GetKeyDown(KeyCode.C))
        {
            byte[] nameBytes = Encoding.UTF8.GetBytes("TestRoom");
            SendPacket(PacketType.CREATEROOM, nameBytes);
        }

        if (Input.GetKeyDown(KeyCode.J))
        {
            byte[] payload = BitConverter.GetBytes(1);
            SendPacket(PacketType.JOIN, payload);
        }

        HandleMovementInput();
    }

    void HandleMovementInput()
    {
        if (myPlayerObject == null) return;

        float moveX = 0f;
        float moveY = 0f;

        if (Input.GetKey(KeyCode.A) || Input.GetKey(KeyCode.LeftArrow)) moveX -= 1f;
        if (Input.GetKey(KeyCode.D) || Input.GetKey(KeyCode.RightArrow)) moveX += 1f;
        if (Input.GetKey(KeyCode.W) || Input.GetKey(KeyCode.UpArrow)) moveY += 1f;
        if (Input.GetKey(KeyCode.S) || Input.GetKey(KeyCode.DownArrow)) moveY -= 1f;

        if (moveX == 0f && moveY == 0f) return;

        float speed = 3f;
        float half = 0.5f;

        float targetX = myX += moveX * speed * Time.deltaTime;
        float targetY = myY += moveY * speed * Time.deltaTime;
        
        if (moveX > 0f)
        {
            int tileX = Mathf.FloorToInt(targetX + half);
            if (!MapRenderer.Instance.IsWalkable(tileX, Mathf.FloorToInt(myY)))
                targetX = tileX - half;
        }
        else if (moveX < 0f)
        {
            int tileX = Mathf.FloorToInt(targetX - half);
            if (!MapRenderer.Instance.IsWalkable(tileX, Mathf.FloorToInt(myY)))
                targetX = tileX + 1f + half;
        }

        if (moveY > 0f)
        {
            int tileY = Mathf.FloorToInt(targetY + half);
            if (!MapRenderer.Instance.IsWalkable(Mathf.FloorToInt(myX), tileY))
                targetY = tileY - half;
        }
        else if (moveY < 0f)
        {
            int tileY = Mathf.FloorToInt(targetY - half);
            if (!MapRenderer.Instance.IsWalkable(Mathf.FloorToInt(myX), tileY))
                targetY = tileY + 1f + half;
        }

        myX = targetX;
        myY = targetY;

        myPlayerObject.transform.position = new Vector3(myX, myY, -1f);
        SendMove(myX, myY);
    }

    public void SendBombPlant()
    {
        Debug.Log("폭탄 설치 요청 전송: " + myX + ", " + myY);
        byte[] payload = new byte[8];
        BitConverter.GetBytes(myX).CopyTo(payload, 0);
        BitConverter.GetBytes(myY).CopyTo(payload, 4);
        SendPacket(PacketType.BOMBPLANT, payload);
    }

    public void SendMove(float x, float y)
    {
        byte[] payload = new byte[8];
        BitConverter.GetBytes(x).CopyTo(payload, 0);
        BitConverter.GetBytes(y).CopyTo(payload, 4);
        SendPacket(PacketType.MOVE, payload);
    }

    private void HandlePacket(ReceivedPacket packet)
    {
        switch (packet.type)
        {
            case PacketType.NOTIFY:
                {
                    Debug.Log("[NOTIFY] " + Encoding.UTF8.GetString(packet.data));
                    break;
                }
            case PacketType.ASSIGNID:
                {
                    myId = BitConverter.ToInt32(packet.data, 0);
                    myRoomId = BitConverter.ToInt32(packet.data, 4);

                    Debug.Log("내 아이디: " + myId + ", 방 번호: " + myRoomId);
                    break;
                }
            case PacketType.MAPINFO:
                {
                    int w = BitConverter.ToInt32(packet.data, 0);
                    int h = BitConverter.ToInt32(packet.data, 4);

                    int[] tiles = new int[w * h];
                    for (int i = 0; i < w * h; i++)
                    {
                        tiles[i] = BitConverter.ToInt32(packet.data, 8 + i * 4);
                    }

                    MapRenderer.Instance.RenderMap(w, h, tiles);
                    break;
                }
            case PacketType.START:
                {
                    Debug.Log("[START] 게임 시작");
                    myPlayerObject = PlayerManager.Instance.SpawnPlayer(myId, 0, 0, true);

                    Camera.main.GetComponent<CameraFollow>().target = myPlayerObject.transform;
                    break;
                }
            case PacketType.MOVE:
                {
                    int userId = BitConverter.ToInt32(packet.data, 0);
                    float x = BitConverter.ToSingle(packet.data, 4);
                    float y = BitConverter.ToSingle(packet.data, 8);

                    if (userId == myId)
                    {
                        myX = x;
                        myY = y;
                        if (myPlayerObject != null)
                        {
                            myPlayerObject.transform.position = new Vector3(myX, myY, -1f);
                        }
                    }
                    else
                    {
                        PlayerManager.Instance.UpdatePlayerPosition(userId, x, y);
                    }
                    break;
                }
            case PacketType.BOMBPLANT:
                {
                    float bx = BitConverter.ToSingle(packet.data, 0);
                    float by = BitConverter.ToSingle(packet.data, 4);
                    Debug.Log("[BOMBPLANT] 수신: " + bx + ", " + by);
                    BombManager.Instance.SpawnBomb(bx, by);
                    break;
                }
            case PacketType.BOOM:
                {
                    int count = BitConverter.ToInt32(packet.data, 0);
                    var tiles = new List<(int, int)>();
                    for (int i = 0; i < count; i++)
                    {
                        int tx = BitConverter.ToInt32(packet.data, 4 + i * 8);
                        int ty = BitConverter.ToInt32(packet.data, 4 + i * 8 + 4);
                        tiles.Add((tx, ty));
                    }

                    BombManager.Instance.RemoveBomb(tiles[0].Item1, tiles[0].Item2);
                    MapRenderer.Instance.ClearTiles(tiles);
                    break;
                }
            case PacketType.PLAYERDIE:
                {
                    int deadId = BitConverter.ToInt32(packet.data, 0);
                    if (deadId == myId)
                    {
                        Debug.Log("나 죽었다");
                        if (myPlayerObject != null) myPlayerObject.SetActive(false);
                    }
                    else
                    {
                        PlayerManager.Instance.SetPlayerDead(deadId);
                    }
                    break;
                }
            case PacketType.END:
                {
                    Debug.Log("[END] " + Encoding.UTF8.GetString(packet.data));
                    break;
                }
            default:
                Debug.Log("Unhandled packet: " + packet.type);
                break;
        }
    }

    public void SendPacket(PacketType type, byte[] data)
    {
        ushort size = (ushort)(4 + data.Length);
        byte[] packet = new byte[size];
        BitConverter.GetBytes((ushort)type).CopyTo(packet, 0);
        BitConverter.GetBytes(size).CopyTo(packet, 2);
        data.CopyTo(packet, 4);
        stream.Write(packet, 0, packet.Length);
    }

    void OnApplicationQuit()
    {
        running = false;
        client?.Close();
    }
}