#include "Scene.hpp"
#include "Engine.hpp"
#include <chrono>
#include <string>
#include <sstream>

//*********************************************************************************************************************
//**                         Scene                                                                                  **
//*********************************************************************************************************************

bool compareGameObjectDepth(const GameObject *a, const GameObject *b)
{
    return a->layer < b->layer;
}
Scene *Scene::m_instance = nullptr;

Scene::Scene() : lastCheckTime(0), checkInterval(5)
{
    m_instance = this;
    timer.start();
    camera.target.x = 0;
    camera.target.y = 0;

    camera.offset.x = 0;
    camera.offset.y = 0;

    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    enableEditor = true;
    enableCollisions = true;
    enableLiveReload = true;
    showDebug = true;
    showStats = true;

    numObjectsRemoved = 0;
    currentMode = None;
    selectedObject = nullptr;
    prevMousePos = {0, 0};
    m_num_layers = 0;
    addLayers(2);
}

Scene::~Scene()
{
    m_instance = nullptr;
}

    void Scene::AddGameObject(GameObject *gameObject)
    {
        gameObject->scene = this;
        gameObject->Render();
        gameObjects.push_back(gameObject);
        addToLayer(gameObject);
    }

    void Scene::AddQueueObject(GameObject *gameObject)
    {
        gameObject->scene = this;
        
        gameObjectsToAdd.push_back(gameObject);
    }

    void Scene::RemoveGameObject(GameObject *gameObject)
    {
        gameObjectsToRemove.push_back(gameObject);
    }

int Scene::addLayer()
{
    std::vector<GameObject *> l;
    l.reserve(4000);
    layers.emplace(layersCount(), l);
    ++m_num_layers;
    return layersCount();
}

void Scene::addToLayer(GameObject *e)
{
    if (e->layer > layersCount())
    {
        while (layersCount() <= e->layer)
        {
            addLayer();
        }
    }
    layers[e->layer].emplace_back(e);
}

int Scene::layersCount()
{
    return (int)layers.size();
}

int Scene::addLayers(int count)
{
    for (int i = 0; i < count; i++)
    {
        addLayer();
    }
    return layersCount();
}

void Scene::ClearScene()
{

    for (int layer = 0; layer < layersCount(); layer++)
    {

        layers[layer].erase(
            std::remove_if(std::begin(layers[layer]), std::end(layers[layer]),
                           [](GameObject *entity)
                           {
                               if (entity->persistent)
                                   return false;
                               return true;
                           }),
            std::end(layers[layer]));
    }

    for (auto gameObject : gameObjectsToAdd)
    {
        gameObject->OnRemove();
        delete gameObject;
        gameObject = nullptr;
    }
    gameObjectsToAdd.clear();

    for (auto gameObject : gameObjects)
    {

        if (!gameObject->persistent)
        {
            numObjectsRemoved++;
            gameObjectsToRemove.push_back(gameObject);
        }
    }

    // auto partition_point = std::partition(gameObjects.begin(), gameObjects.end(),
    //                                         [](GameObject *obj) { return !obj->persistent; });
    // for (auto it = partition_point; it != gameObjects.end(); ++it)
    // {
    //     delete *it;
    // }
    // gameObjects.erase(partition_point, gameObjects.end());


}

void Scene::ClearAndFree()
{
    Log(LOG_INFO, "Clearing and free scene GameObject");

    for (int layer = 0; layer < layersCount(); layer++)
    {
        layers[layer].clear();
    }
    layers.clear();

    for (auto gameObject : gameObjects)
    {
        gameObject->OnRemove();
        delete gameObject;
        gameObject = nullptr;
    }
    gameObjects.clear();

    for (auto gameObject : gameObjectsToRemove)
    {
        delete gameObject;
        gameObject = nullptr;
    }
    gameObjectsToRemove.clear();

    for (auto gameObject : gameObjectsToAdd)
    {
        delete gameObject;
        gameObject = nullptr;
    }
    gameObjectsToAdd.clear();
}

void Scene::Init(const std::string &title, float fps, int windowWidth, int windowHeight, bool fullscreen)
{
    this->title = title;
    this->fps = fps;
    this->windowSize.x = windowWidth;
    this->windowSize.y = windowHeight;
    this->fullscreen = fullscreen;
    cameraBounds.x = 0;
    cameraBounds.y = 0;
    cameraBounds.width = windowWidth;
    cameraBounds.height = windowHeight;

   
}

void Scene::SetWorld(float width, float height)
{
    this->worldSize.x = width;
    this->worldSize.y = height;

}

void Scene::SetBackground(int r, int g, int b)
{
    background.r = r;
    background.g = g;
    background.b = b;
}


float Distance(const Vec2 &a, const Vec2 &b)
{
    return sqrt(pow(b.x - a.x, 2) + pow(b.y - a.y, 2));
}
GameObject *Scene::CirclePick(float x, float y, float radius)
{

    for (GameObject *obj : gameObjects)
    {
        if (!obj->pickable)
            continue;
        if (CheckCollisionCircleRec({x, y}, radius, obj->bound))
            return obj;
    }
    return nullptr;
}

GameObject *Scene::MousePick()
{

    for (GameObject *obj : gameObjects)
    {
        if (!obj->pickable)
            continue;
        if (CheckCollisionPointRec(GetMousePosition(), obj->bound))
            return obj;
    }
    return nullptr;
}

GameObject *Scene::RectanglePick(float x, float y, float width, float height)
{

    for (GameObject *obj : gameObjects)
    {
        if (!obj->pickable)
            continue;
        if (CheckCollisionRecs({x, y, width, height}, obj->bound))
            return obj;
    }
    return nullptr;
}

void Scene::Editor()
{

    Vec2 mousePosition = Vec2(GetMouseX(), GetMouseY());
    Vector2 vmousePosition = GetMousePosition();

    if (selectedObject != nullptr)
    {
        if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON))
        {
            selectedObject->OnReady();
            selectedObject = nullptr;
            return;
        }
    }

    if (IsKeyPressed(KEY_S))
        currentMode = Scale;
    else if (IsKeyPressed(KEY_M))
        currentMode = Move;
    else if (IsKeyPressed(KEY_R))
        currentMode = Rotate;
    else if (IsKeyReleased(KEY_S) || IsKeyReleased(KEY_M) || IsKeyReleased(KEY_R))
        currentMode = None;

    DrawText("S - Scale", GetScreenWidth() - 150, 10, 20, (currentMode == Scale ? RED : WHITE));
    DrawText("M - Move", GetScreenWidth() - 150, 30, 20, (currentMode == Move ? RED : WHITE));
    DrawText("R - Rotate", GetScreenWidth() - 150, 50, 20, (currentMode == Rotate ? RED : WHITE));
    if (selectedObject != nullptr)
    {
        DrawText(TextFormat("Select %s", selectedObject->name.c_str()), GetScreenWidth() - 150, 70, 20, RED);
    }

    for (auto gameObject : gameObjects)
    {

        for (auto gameObject2 : gameObject->children)
        {
            Rectangle rect2 = gameObject2->bound;

            if (CheckCollisionPointRec(vmousePosition, rect2) && !selectedObject)
            {
                DrawRectangleLinesEx(rect2, 2, RED);
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                {
                    initialObjectPosition = gameObject2->transform->position;
                    initialMousePosition = mousePosition;
                    selectedObject = gameObject2;
                    break;
                }
            }
        }

        Rectangle rect = gameObject->bound;

        if (CheckCollisionPointRec(vmousePosition, rect) && !selectedObject)
        {
            DrawRectangleLinesEx(rect, 2, RED);
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                initialObjectPosition = gameObject->transform->position;
                initialMousePosition = mousePosition;

                selectedObject = gameObject;

                break;
            }
        }
    }

    if (selectedObject != nullptr)
    {

        Vector2 objectCenter = {
            selectedObject->getX() + selectedObject->bound.width / 2,
            selectedObject->getY() + selectedObject->bound.height / 2};

        DrawRectangleLinesEx(selectedObject->bound, 2, GREEN);

        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
        {

            switch (currentMode)
            {
            case Move:
            {

                selectedObject->transform->position.x += mousePosition.x - prevMousePos.x;
                selectedObject->transform->position.y += mousePosition.y - prevMousePos.y;
            }
            break;
            case Scale:
            {

                float distanceDelta = mousePosition.x - prevMousePos.x;
                float scaleIncrement = distanceDelta / 100.0f;
                selectedObject->transform->scale += scaleIncrement;
            }
            break;
            case Rotate:
            {
                float prevAngle = atan2(prevMousePos.y - objectCenter.y, prevMousePos.x - objectCenter.x);
                float currAngle = atan2(mousePosition.y - objectCenter.y, mousePosition.x - objectCenter.x);
                selectedObject->transform->rotation += (currAngle - prevAngle) * RAD2DEG;
            }
            break;
            case None:
            default:
                break;
            }

            if (selectedObject->transform->scale.x < 0.1f)
                selectedObject->transform->scale.x = 0.1f;
            if (selectedObject->transform->scale.y < 0.1f)
                selectedObject->transform->scale.y = 0.1f;

            selectedObject->UpdateWorld();
        }
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
        {
        }
    }
    prevMousePos = mousePosition;
}

std::string formatSize(size_t size)
{
    constexpr double KB = 1024.0;
    constexpr double MB = 1024.0 * 1024.0;

    std::string formattedSize;

    if (size < KB)
    {
        formattedSize = std::to_string(size) + " B";
    }
    else if (size < MB)
    {
        formattedSize = std::to_string(round(size / KB * 100.0) / 100.0) + " KB";
    }
    else
    {
        formattedSize = std::to_string(round(size / MB * 100.0) / 100.0) + " MB";
    }

    return formattedSize;
}

bool Scene::inView(const  Rectangle& r )
{
    return CheckCollisionRecs(r, cameraView);
}

void Scene::Render()
{



    ClearBackground(background);
    BeginMode2D(camera);
    cameraPoint.x = (camera.offset.x - camera.target.x) ;
    cameraPoint.y = (camera.offset.y - camera.target.y) ;

    for (int i = 0; i < (int)layers.size(); i++)
    {
        for (auto &e : layers[i])
        {
            if (e->alive && e->visible && inView(e->bound))
            {
                e->Render();
                objectRender++;
            }
        }
    }

    if (showDebug)
    {
        for (auto gameObject : gameObjects)
        {
            if (gameObject->visible && gameObject->active)
                gameObject->Debug();
        }
    }

    if (timer.isPaused())
    {

        for (auto gameObject : gameObjects)
        {
            if (gameObject->alive && gameObject->active)
            {
                gameObject->OnPause();
            }
        }

        if (enableEditor)
            Editor();
    }

   






    EndMode2D();
    // int lastLayerKey = layers.rbegin()->first;
    // for (auto &e : layers[lastLayerKey])
    // {
    //     if (e->alive && e->visible)
    //     {
    //         e->Render();
    //     }
    // }

  
    

    if (showStats)
    {
        int Y = GetScreenHeight() - 40;
        int index = 0;
        for (int i = 0; i < layersCount(); i++)
        {
            if (layers[i].size() > 0)
            {
                Y = GetScreenHeight() - 24 - index * 22;
                index++;
            }
        }
        DrawRectangle(10, Y , 170, index * 22, Fade(SKYBLUE, 0.5f));
        DrawRectangleLines(11, Y+1, 168, index * 22-2, BLUE);
        Y = GetScreenHeight() - 40;
        index = 0;
        for (int i = 0; i < layersCount(); i++)
        {
            if (layers[i].size() > 0)
            {
                Y = GetScreenHeight() - 20 - index * 22;
                DrawText(TextFormat("Layer [%d]  Objects [%d] ", i, layers[i].size()), 28, Y, 10, LIME);
                index++;
            }
        }

        float x = 15;
        float y = 18;
        float s = 18;

        DrawRectangle(10, 10, 220, 100, BLACK);
        DrawRectangle(10, 10, 220, 100, Fade(SKYBLUE, 0.5f));
        DrawRectangleLines(10, 10, 220, 100, BLUE);

        DrawFPS(x, y);
        DrawText(TextFormat("Objects: %i/%d", gameObjects.size(),objectRender), x, y + 1 * s, s, LIME);
        DrawText(TextFormat("Elapsed time: %.2f", timer.getElapsedTime()), x, y + 2 * s, s, LIME);
        DrawText(TextFormat("Delta time: %.2f", timer.getDeltaTime()), x, y + 3 * s, s, LIME);
            //  DrawText(TextFormat("View: %f %f %f %f", cameraView.x,cameraView.y,cameraView.width,cameraView.height), x, y + 5 * s, s, LIME);
     //   DrawText(TextFormat("Camera: %f %f %f %f", camera.target.x,camera.target.y,camera.offset.x,camera.offset.y), x, y + 6 * s, s, LIME);



    }
}

void Scene::LiveReload()
{

    if (!enableLiveReload)
        return;
    std::time_t currentTime = std::time(nullptr);

    if (currentTime - lastCheckTime >= checkInterval)
    {
        lastCheckTime = currentTime;
        //  Log(LOG_INFO, "Checking for script changes");
        // for (auto gameObject : gameObjects)
        // {
        //     if (!gameObject->script)
        //     {
        //         continue;
        //     }
        //     gameObject->LiveReload();
        // }
    }
}

GameObject *Scene::GetGameObjectByName(const std::string &name)
{
    for (auto gameObject : gameObjects)
    {
        if (gameObject->name == name)
        {
            return gameObject;
        }
    }
    Log(LOG_WARNING, "GameObject %s not found", name.c_str());
    return nullptr;
}

void Scene::Update()
{
    timer.update();

    objectRender=0;
    cameraView.x= (-camera.offset.x/camera.zoom) + camera.target.x - (windowSize.x/2.0f/camera.zoom);
    cameraView.y= (-camera.offset.y/camera.zoom) + camera.target.y - (windowSize.y/2.0f/camera.zoom);
    cameraView.width = (float)(windowSize.x/camera.zoom)+(camera.offset.x/camera.zoom) ;
    cameraView.height= (float)(windowSize.y/camera.zoom)+(camera.offset.y/camera.zoom) ;
 

    if (IsKeyReleased(KEY_F1))
    {
        showDebug = !showDebug;
    }

    if (IsKeyPressed(KEY_P))
    {
        if (selectedObject != nullptr)
        {
            selectedObject->OnReady();
            selectedObject = nullptr;
        }
        if (timer.isPaused())
        {
            timer.resume();
        }
        else
        {
            timer.pause();
        }
    }


    if (IsKeyReleased(KEY_F6))
    {
        enableLiveReload = !enableLiveReload;
        if (enableLiveReload)
            Log(LOG_INFO, "Live reload enabled");
        else
            Log(LOG_INFO, "Live reload disabled");
    }

    if (IsKeyReleased(KEY_F3))
    {
        enableCollisions = !enableCollisions;
        if (enableCollisions)
            Log(LOG_INFO, "Collisions enabled");
        else
            Log(LOG_INFO, "Collisions disabled");
    }

    // if (needSort)
    // {
    //     std::sort(gameObjects.begin(), gameObjects.end(), compareGameObjectDepth);
    //     needSort = false;
    // }

    if (!timer.isPaused())
    {
        for (auto gameObject : gameObjects)
        {
            if (gameObject->alive && gameObject->active)
            {
                gameObject->Update(timer.getDeltaTime());
            }
            if (!gameObject->alive)
            {
                numObjectsRemoved++;
                gameObjectsToRemove.push_back(gameObject);
            }
        }
    }

  

    for (auto gameObject : gameObjectsToRemove)
    {
        int layerKey = gameObject->layer;
        if (layers.find(layerKey) != layers.end())
        {
            std::vector<GameObject *> &objectsInLayer = layers[layerKey];
            auto it = std::find(objectsInLayer.begin(), objectsInLayer.end(), gameObject);
            if (it != objectsInLayer.end())
            {
                objectsInLayer.erase(it);
            }
        }

        auto it = std::find(gameObjects.begin(), gameObjects.end(), gameObject);
        if (it != gameObjects.end())
        {
            gameObject->OnRemove();
            gameObjects.erase(it);
            gameObject->scene = nullptr;
            delete gameObject;
            gameObject = nullptr;
        }
    }
    gameObjectsToRemove.clear();

    for (auto gameObject : gameObjectsToAdd)
    {
        gameObject->UpdateWorld();
        AddGameObject(gameObject);
    }
    gameObjectsToAdd.clear();
    LiveReload();
    if (enableCollisions)
        Collision();


    if (IsKeyReleased(KEY_F2))
    {
        ClearScene();
    }
}
bool Scene::place_meeting_layer(GameObject *obj, float x, float y, int layer)
{
    if (!obj->collidable)
        return false;

    for (auto other : layers[layer])
    {
        if (!other->collidable)
            continue;
        if (obj->collideWith(other, x, y))
            return true;
    }
    return false;
}
bool Scene::place_meeting(GameObject *obj, float x, float y, const std::string &objname)
{
    if (!obj->collidable )
        return false;
    
    for (auto other : gameObjects)
    {
        if ( (!other->collidable) && !inView(other->bound))
            continue;

        if (strcmp(other->name.c_str(), objname.c_str())==0)
        {
            if (obj->collideWith(other, x, y))
                return true;
        } else
            continue;        
    }
    return false;

    // for (auto gameObject : gameObjects)
    // {
    //     if (gameObject->name == name)
    //     {
    //         if (gameObject->HasComponent<BoxColiderComponent>())
    //         {
    //             auto boxColiderComponent = gameObject->GetComponent<BoxColiderComponent>();
    //             if (boxColiderComponent != nullptr)
    //             {
    //                 if (CheckCollisionPointRec({x, y}, boxColiderComponent->rect))
    //                 {
    //                     return true;
    //                 }
    //             }
    //         }

    //         if (gameObject->HasComponent<CircleColiderComponent>())
    //         {
    //             auto circleColiderComponent = gameObject->GetComponent<CircleColiderComponent>();
    //             if (circleColiderComponent != nullptr)
    //             {
    //                 if (CheckCollisionPointCircle({x, y}, circleColiderComponent->center, circleColiderComponent->radius))
    //                 {
    //                     return true;
    //                 }
    //             }
    //         }
    //     }
    // }
}

bool Scene::place_free(GameObject *obj, float x, float y)
{ 
    if (!obj->collidable )
        return true;
    
    for (auto other : gameObjects)
    {
        
        if ( (other->collidable))
        {
            if (obj->collideWith(other, x, y))
                return false;
        } else
             continue;
        
    }
    return true;

    // for (auto gameObject : gameObjects)
    // {
    //     if (gameObject->HasComponent<BoxColiderComponent>())
    //     {
    //         auto boxColiderComponent = gameObject->GetComponent<BoxColiderComponent>();
    //         if (boxColiderComponent != nullptr)
    //         {
    //             if (CheckCollisionPointRec({x, y}, boxColiderComponent->rect))
    //             {
    //                 return false;
    //             }
    //         }
    //     }

    //     if (gameObject->HasComponent<CircleColiderComponent>())
    //     {

    //         auto circleColiderComponent = gameObject->GetComponent<CircleColiderComponent>();
    //         if (circleColiderComponent != nullptr)
    //         {
    //             if (CheckCollisionPointCircle({x, y}, circleColiderComponent->center, circleColiderComponent->radius))
    //             {
    //                 return false;
    //             }
    //         }
    //     }
    // }
    return true;
}

void Scene::SetPositionCamera(float x, float y)
{
    camera.target.x = x;
    camera.target.y = y;
    cameraBounds.x = x;
    cameraBounds.y = y;
}

void Scene::SetCamera(float x, float y)
{

    camera.target.x = x;
    camera.target.y = y;
    cameraBounds.x = x - camera.offset.x * camera.zoom;
    cameraBounds.y = y - camera.offset.y * camera.zoom;

    //    camera.target.x  = 2 * camera.offset.x - x;
    //    camera.target.y  = 2 * camera.offset.y - y;
    //    cameraBounds.x = camera.target.x;
    //    cameraBounds.y = camera.target.y;

    // //local targetX = 2 * (WindowWidth /2)- self.x
    // //local targetY = 2 * (WindowHeight/2)- self.y
}

void Scene::Collision()
{
    for (int i = 0; i < (int)gameObjects.size(); i++)
    {
        GameObject *a = gameObjects[i];
        GameObject *parentA = a->parent;
        if (!a->collidable)
            continue;

        for (int j = i + 1; j < (int)gameObjects.size(); j++)
        {

            GameObject *b = gameObjects[j];

            if (!b->collidable)
                continue;

            if (a == b || b == parentA || a == b->parent)
            {
                continue;
            }

            // Log(LOG_INFO, "Collision between %s and %s", a->name.c_str(), b->name.c_str());

            if (a->HasComponent<BoxColiderComponent>() && b->HasComponent<BoxColiderComponent>())
            {
                BoxColiderComponent *colliderA = a->GetComponent<BoxColiderComponent>();
                BoxColiderComponent *colliderB = b->GetComponent<BoxColiderComponent>();

                if (colliderA->IsColide(colliderB))
                {
                    colliderA->OnColide(colliderB);
                    colliderB->OnColide(colliderA);
                    //          Log(LOG_INFO, "Collision between A %s and B %s", a->name.c_str(), b->name.c_str());
                    return;
                }
                if (colliderB->IsColide(colliderA))
                {
                    colliderB->OnColide(colliderA);
                    colliderA->OnColide(colliderB);
                    //   Log(LOG_INFO, "Collision between B %s and A %s", b->name.c_str(), a->name.c_str());
                    return;
                }
            }
            else if (a->HasComponent<CircleColiderComponent>() && b->HasComponent<CircleColiderComponent>())
            {
                CircleColiderComponent *colliderA = a->GetComponent<CircleColiderComponent>();
                CircleColiderComponent *colliderB = b->GetComponent<CircleColiderComponent>();

                if (colliderA->IsColide(colliderB))
                {
                    colliderA->OnColide(colliderB);
                    colliderB->OnColide(colliderA);
                    //          Log(LOG_INFO, "Collision between A %s and B %s", a->name.c_str(), b->name.c_str());
                    return;
                }
                if (colliderB->IsColide(colliderA))
                {
                    colliderB->OnColide(colliderA);
                    colliderA->OnColide(colliderB);
                    //   Log(LOG_INFO, "Collision between B %s and A %s", b->name.c_str(), a->name.c_str());
                    return;
                }
            }
            else if (a->HasComponent<BoxColiderComponent>() && b->HasComponent<CircleColiderComponent>())
            {
                BoxColiderComponent *colliderA = a->GetComponent<BoxColiderComponent>();
                CircleColiderComponent *colliderB = b->GetComponent<CircleColiderComponent>();
                if (colliderA->IsColide(colliderB))
                {
                    colliderA->OnColide(colliderB);
                    colliderB->OnColide(colliderA);
                    //          Log(LOG_INFO, "Collision between A %s and B %s", a->name.c_str(), b->name.c_str());
                    return;
                }
                if (colliderB->IsColide(colliderA))
                {
                    colliderB->OnColide(colliderA);
                    colliderA->OnColide(colliderB);
                    //   Log(LOG_INFO, "Collision between B %s and A %s", b->name.c_str(), a->name.c_str());
                    return;
                }
            }
            else if (a->HasComponent<CircleColiderComponent>() && b->HasComponent<BoxColiderComponent>())
            {
                CircleColiderComponent *colliderA = a->GetComponent<CircleColiderComponent>();
                BoxColiderComponent *colliderB = b->GetComponent<BoxColiderComponent>();
                if (colliderA->IsColide(colliderB))
                {
                    colliderA->OnColide(colliderB);
                    colliderB->OnColide(colliderA);
                    //          Log(LOG_INFO, "Collision between A %s and B %s", a->name.c_str(), b->name.c_str());
                    return;
                }
                if (colliderB->IsColide(colliderA))
                {
                    colliderB->OnColide(colliderA);
                    colliderA->OnColide(colliderB);
                    //   Log(LOG_INFO, "Collision between B %s and A %s", b->name.c_str(), a->name.c_str());
                    return;
                }
            }
        }
    }
}

Vector2 ColideComponent::GetWorldPosition()
{
    Vector2 pos;
    pos.x = object->getWorldX();
    pos.y = object->getWorldY();
    return pos;
}

Vector2 BoxColiderComponent::GetWorldPosition()
{
    Vector2 pos;
    pos.x = object->getWorldX() + rect.x;
    pos.y = object->getWorldY() + rect.y;
    return pos;
}

Vector2 CircleColiderComponent::GetWorldPosition()
{
    Vector2 pos;
    pos.x = object->getWorldX() + center.x;
    pos.y = object->getWorldY() + center.y;
    return pos;
}

bool BoxColiderComponent::IsColide(ColideComponent *other)
{
    if (other->type == ColliderType::Box)
    {
        BoxColiderComponent *box = (BoxColiderComponent *)other;
        return CheckCollisionRecs(GetWorldRect(), box->GetWorldRect());
    }
    else if (other->type == ColliderType::Circle)
    {
        CircleColiderComponent *circle = (CircleColiderComponent *)other;
        return CheckCollisionCircleRec(circle->center, circle->radius, GetWorldRect());
    }
    return false;
}

void BoxColiderComponent::OnColide(ColideComponent *other)
{
    other->object->OnCollision(this->object);
    object->OnCollision(other->object);
    //  Log(LOG_INFO, "BoxColiderComponent OnColide");
}

void BoxColiderComponent::OnDebug()
{
    DrawRectangleLinesEx(GetWorldRect(), 2, LIME);
}

Rectangle BoxColiderComponent::GetWorldRect()
{
    Rectangle worldRect;
    worldRect.x = object->getWorldX() + rect.x;
    worldRect.y = object->getWorldY() + rect.y;
    worldRect.width = rect.width;
    worldRect.height = rect.height;
    return worldRect;
}

bool CircleColiderComponent::IsColide(ColideComponent *other)
{
    if (other->type == ColliderType::Box)
    {
        BoxColiderComponent *box = (BoxColiderComponent *)other;
        return CheckCollisionCircleRec(GetWorldPosition(), radius, box->GetWorldRect());
    }
    else if (other->type == ColliderType::Circle)
    {
        CircleColiderComponent *circle = (CircleColiderComponent *)other;
        return CheckCollisionCircles(GetWorldPosition(), radius, circle->GetWorldPosition(), circle->radius);
    }
    return false;
}

void CircleColiderComponent::OnColide(ColideComponent *other)
{
    other->object->OnCollision(this->object);
    object->OnCollision(other->object);
    // Log(LOG_INFO, "CircleColiderComponent OnColide");
}

void CircleColiderComponent::OnDebug()
{
    Vector2 p = GetWorldPosition();
    DrawCircleLines(p.x, p.y, radius, RED);
}

void CircleColiderComponent::OnInit()
{
    if (object)
    {
        object->collidable = true;
    }
}

void BoxColiderComponent::OnInit()
{
    if (object)
    {
        object->collidable = true;
    }
}

//**************************************************************************************************
//  ColideComponent
//**************************************************************************************************

bool Contains(Rectangle src, Rectangle obj)
{
    float r1 = src.x + src.width;
    float r2 = obj.x + obj.width;
    float b1 = src.y + src.height;
    float b2 = obj.y + obj.height;

    if (src.x < r2 && r1 > obj.x && src.y < b2 && b1 > obj.y)
    {
        return true;
    }
    return false;
}

QuadtreeNode::QuadtreeNode(int level, const AABB &bounds) : level(level), bounds(bounds)
{
    for (int i = 0; i < 4; i++)
    {
        children[i] = nullptr;
    }
}

QuadtreeNode::~QuadtreeNode()
{
    for (int i = 0; i < 4; i++)
    {
        delete children[i];
    }
}

void QuadtreeNode::split()
{
    float subWidth = bounds.m_w / 2.0f;
    float subHeight = bounds.m_h / 2.0f;
    float x = bounds.m_x;
    float y = bounds.m_y;

    children[0] = new QuadtreeNode(level + 1, AABB(x + subWidth, y, subWidth, subHeight));
    children[1] = new QuadtreeNode(level + 1, AABB(x, y, subWidth, subHeight));
    children[2] = new QuadtreeNode(level + 1, AABB(x, y + subHeight, subWidth, subHeight));
    children[3] = new QuadtreeNode(level + 1, AABB(x + subWidth, y + subHeight, subWidth, subHeight));
}

int QuadtreeNode::getIndex(const AABB &aabb) const
{
    int index = -1;
    float verticalMidpoint = bounds.m_x + (bounds.m_w / 2.0f);
    float horizontalMidpoint = bounds.m_y + (bounds.m_h / 2.0f);

    bool topQuadrant = aabb.m_y < horizontalMidpoint && (aabb.m_y + aabb.m_y) < horizontalMidpoint;
    bool bottomQuadrant = aabb.m_y > horizontalMidpoint;

    if (aabb.m_x < verticalMidpoint && (aabb.m_x + aabb.m_w) < verticalMidpoint)
    {
        if (topQuadrant)
        {
            index = 1;
        }
        else if (bottomQuadrant)
        {
            index = 2;
        }
    }
    else if (aabb.m_x > verticalMidpoint)
    {
        if (topQuadrant)
        {
            index = 0;
        }
        else if (bottomQuadrant)
        {
            index = 3;
        }
    }

    return index;
}

void QuadtreeNode::insert(GameObject *obj)
{
    if (children[0] != nullptr)
    {
        int index = getIndex(obj->GetAABB());

        if (index != -1)
        {
            children[index]->insert(obj);
            return;
        }
    }

    objects.push_back(obj);

    if (objects.size() > MAX_OBJECTS && level < MAX_LEVELS)
    {
        if (children[0] == nullptr)
        {
            split();
        }

        auto it = objects.begin();
        while (it != objects.end())
        {
            int index = getIndex((*it)->GetAABB());
            if (index != -1)
            {
                children[index]->insert(*it);
                it = objects.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
}
void QuadtreeNode::remove(GameObject *obj)
{
    int index = getIndex(obj->GetAABB());
    if (children[0] != nullptr && index != -1)
    {
        children[index]->remove(obj);
        return;
    }

    for (auto it = objects.begin(); it != objects.end(); ++it)
    {
        if (*it == obj)
        {
            objects.erase(it);
            break;
        }
    }

    // Combina os quadrantes filhos, se necessário
    if (children[0] != nullptr)
    {
        int totalObjects = 0;
        for (int i = 0; i < 4; i++)
        {
            totalObjects += children[i]->objects.size();
        }

        if (totalObjects <= MAX_OBJECTS)
        {
            for (int i = 0; i < 4; i++)
            {
                objects.insert(objects.end(), children[i]->objects.begin(), children[i]->objects.end());
                delete children[i];
                children[i] = nullptr;
            }
        }
    }
}
void QuadtreeNode::draw()
{
    DrawRectangleLines(bounds.m_x, bounds.m_y, bounds.m_w, bounds.m_h, RAYWHITE);

    if (children[0] != nullptr)
    {
        for (int i = 0; i < 4; i++)
        {
            children[i]->draw();
        }
    }
}
void QuadtreeNode::retrieve(std::vector<GameObject *> &returnObjects, const Vec2 &point)
{
    if (bounds.contains(point))
    {
        returnObjects.insert(returnObjects.end(), objects.begin(), objects.end());
        //   DrawRectangleLines(bounds.m_x, bounds.m_y, bounds.m_w, bounds.m_h, RED);

        if (children[0] != nullptr)
        {
            for (int i = 0; i < 4; i++)
            {
                children[i]->retrieve(returnObjects, point);
            }
        }
    }
}
void QuadtreeNode::retrieve(std::vector<GameObject *> &returnObjects, const AABB &queryAABB)
{
    if (bounds.intersects(queryAABB))
    {
        for (GameObject *obj : objects)
        {
            if (obj->GetAABB().intersects(queryAABB))
            {
                // AABB aabb = obj->GetAABB();
                // DrawRectangle(aabb.m_x, aabb.m_y, aabb.m_w, aabb.m_h, BLUE);
                returnObjects.push_back(obj);
            }
        }

        if (children[0] != nullptr)
        {
            for (int i = 0; i < 4; i++)
            {
                children[i]->retrieve(returnObjects, queryAABB);
            }
        }
    }
}
void QuadtreeNode::retrieve(const Vec2 &center, float radius, std::vector<GameObject *> &resultObjects) const
{
    if (AABB::IntersectsCircle(bounds, center, radius))
    {
        for (const GameObject *obj : objects)
        {
            if (AABB::IntersectsCircle(obj->GetAABB(), center, radius))
            {
                // resultObjects.push_back(obj);
                resultObjects.push_back(const_cast<GameObject *>(obj));
            }
        }

        if (children[0] != nullptr)
        {
            for (int i = 0; i < 4; i++)
            {
                children[i]->retrieve(center, radius, resultObjects);
            }
        }
    }
}

int QuadtreeNode::countObjects() const
{
    int count = objects.size();

    if (children[0] != nullptr)
    {
        for (int i = 0; i < 4; i++)
        {
            count += children[i]->countObjects();
        }
    }

    return count;
}
void QuadtreeNode::clear()
{
    objects.clear();

    if (children[0] != nullptr)
    {
        for (int i = 0; i < 4; i++)
        {
            children[i]->clear();
            delete children[i];
            children[i] = nullptr;
        }
    }
}

//**********************************************************************************************//
//                                                                                              //
//  Quadtree.h                                                                                  //
//************************************************************************************************
Quadtree::Quadtree(float x, float y, float width, float height)
{
    root = new QuadtreeNode(0, AABB(x, y, width, height));
}

Quadtree::~Quadtree()
{
    delete root;
}

void Quadtree::insert(GameObject *obj)
{
    root->insert(obj);
}

void Quadtree::remove(GameObject *obj)
{
    root->remove(obj);
}
void Quadtree::draw()
{
    root->draw();
}
std::vector<GameObject *> Quadtree::getObjectsAtPoint(const Vec2 &point)
{
    std::vector<GameObject *> returnObjects;
    root->retrieve(returnObjects, point);
    return returnObjects;
}
std::vector<GameObject *> Quadtree::getObjectsInAABB(const AABB &queryAABB)
{
    std::vector<GameObject *> returnObjects;
    root->retrieve(returnObjects, queryAABB);
    return returnObjects;
}
std::vector<GameObject *> Quadtree::getObjectsInCircle(const Vec2 &center, float radius)
{
    std::vector<GameObject *> resultObjects;
    root->retrieve(center, radius, resultObjects);
    return resultObjects;
}

int Quadtree::countObjects() const
{
    return root->countObjects();
}
void Quadtree::clear()
{
    root->clear();
}