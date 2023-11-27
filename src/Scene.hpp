#pragma once
#include <raylib.h>
#include "Utils.hpp"



//*********************************************************************************************************************
//**                         Scene                                                                                   **
//*********************************************************************************************************************
class GameObject;
class SpriteComponent;
class TransformComponent;

class QuadtreeNode
{
public:
    static const int MAX_OBJECTS = 4;
    static const int MAX_LEVELS = 5;

    QuadtreeNode(int level, const AABB &bounds);
    ~QuadtreeNode();
    void split();

    int getIndex(const AABB &aabb) const;
    void insert(GameObject *obj);
    void remove(GameObject *obj);
    void draw();
    void retrieve(std::vector<GameObject *> &returnObjects, const Vec2 &point);

    void retrieve(std::vector<GameObject *> &returnObjects, const AABB &queryAABB);

    void retrieve(const Vec2 &center, float radius, std::vector<GameObject *> &resultObjects) const;

    int countObjects() const;
    void clear();

    int level;
    AABB bounds;
    std::vector<GameObject *> objects;
    QuadtreeNode *children[4];
};

class Quadtree
{
public:
    QuadtreeNode *root;

    Quadtree(float x, float y, float width, float height);

    ~Quadtree();

    void insert(GameObject *obj);

    void remove(GameObject *obj);

    void draw();

    std::vector<GameObject *> getObjectsAtPoint(const Vec2 &point);

    std::vector<GameObject *> getObjectsInAABB(const AABB &queryAABB);

    std::vector<GameObject *> getObjectsInCircle(const Vec2 &center, float radius);

    int countObjects() const;

    void clear();
};

class Scene
{
public:
    enum TransformMode
    {
        None,
        Scale,
        Move,
        Rotate
    };

    Scene();
    virtual ~Scene();

    static Scene *Instance() { return m_instance; }
    /*
    remove all gameobjects from scene  free memory
    */
    void ClearAndFree();
    /*
    remove all gameobjects from scene not the persistent objects
    */
    void ClearScene();
    void LiveReload();

    // create layers
    int addLayer();
    int addLayers(int count);
    void addToLayer(GameObject *e);
    int layersCount();




    GameObject *CirclePick(float x, float y, float radius);
    GameObject *MousePick();
    GameObject *RectanglePick(float x, float y, float w, float h);

    void Editor();
    void SetCamera(float x, float y);
    void SetPositionCamera(float x, float y);

    void AddGameObject(GameObject *gameObject); 

    void AddQueueObject(GameObject *gameObject);
  

    void RemoveGameObject(GameObject *gameObject);
  

    GameObject *GetGameObjectByName(const std::string &name);
    bool inView(const  Rectangle& r );

    void Update();
    void Render();
    void Collision();

    void Init(const std::string &title, float fps, int windowWidth, int windowHeight, bool fullscreen);
    void SetWorld(float width, float height);
    void SetBackground(int r, int g, int b);

    bool place_meeting(GameObject *obj, float x, float y, const std::string &name);
    bool place_meeting_layer(GameObject *obj, float x, float y, int layer);
    bool place_free(GameObject *obj, float x, float y);

    std::vector<GameObject *> gameObjects;
    std::vector<GameObject *> gameObjectsToRemove;
    std::vector<GameObject *> gameObjectsToAdd;
    std::vector<GameObject *> gameObjectsZoombies; // objects sem pai e sem estar na scene
    std::map<int, std::vector<GameObject *>> layers;
    int m_num_layers;

    int numObjectsRemoved;
    bool needSort;
    bool enableLiveReload;
    bool enableCollisions;
    bool showDebug;
    bool enableEditor;
    bool showStats;
    int objectRender;
    
 
    Camera2D camera;
    Rectangle cameraBounds;
    Rectangle cameraView;
    Vector2   cameraPoint; 
    Timer timer;
    std::time_t lastCheckTime;
    std::time_t checkInterval;
    TransformMode currentMode;
    GameObject *selectedObject;
    Vec2 initialObjectPosition;
    Vec2 initialMousePosition;
    Vec2 prevMousePos;
    Vec2 windowSize;
    Vec2 worldSize;
    float fps;
    std::string title;
    Color background;
    bool fullscreen;
    static Scene *m_instance;
};
