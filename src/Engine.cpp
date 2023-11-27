#include "Scene.hpp"
#include "Engine.hpp"
#include <string>
#include <sstream>


ComponentID GetUniqueComponentID() noexcept
{
    static ComponentID lastID{0u};
    return lastID++;
}

//*********************************************************************************************************************
//**                         TransformComponent                                                                              **
//*********************************************************************************************************************

TransformComponent::TransformComponent(GameObject *parent) : object(parent)
{
    // Log(LOG_INFO, "TransformComponent::TransformComponent()");
    this->rotation = 0;
    this->position = Vec2(0.0f);
    this->scale = Vec2(1.0f);
    this->skew = Vec2(0.0f);
    transform.Identity();
}

TransformComponent::~TransformComponent()
{
    // Log(LOG_INFO, "TransformComponent::~TransformComponent()");
    object = nullptr;
}

void TransformComponent::pointToMouse(float speed, float angleDiff)
{
    Vector2 v = GetMousePosition();
    TurnTo(v.x, v.y, speed, angleDiff);
}

void TransformComponent::TurnTo(float x, float y, float speed, float angleDiff)
{

    rotation = lerpAngleDegrees(rotation, getAngle(position.x, position.y, x, y) + angleDiff, speed);
}

Matrix2D TransformComponent::GetLocalTrasformation()
{

    local_transform.Identity();
    if (skew.x == 0.0f && skew.y == 0.0f)
    {

        if (rotation == 0.0)
        {

            local_transform.Set(scale.x, 0.0, 0.0, scale.y, position.x - pivot.x * scale.x, position.y - pivot.y * scale.y);
        }
        else
        {
            float acos = cos(rotation * RAD);
            float asin = sin(rotation * RAD);
            float a = scale.x * acos;
            float b = scale.x * asin;
            float c = scale.y * -asin;
            float d = scale.y * acos;
            float tx = position.x - pivot.x * a - pivot.y * c;
            float ty = position.y - pivot.x * b - pivot.y * d;

            local_transform.Set(a, b, c, d, tx, ty);
        }
    }
    else
    {

        local_transform.Identity();
        local_transform.Scale(scale.x, scale.y);
        local_transform.Skew(skew.x, skew.y);
        local_transform.Rotate(rotation);
        local_transform.Translate(position.x, position.y);

        if (pivot.x != 0.0f || pivot.y != 0.0f)
        {

            local_transform.tx = position.x - local_transform.a * pivot.x - local_transform.c * pivot.y;
            local_transform.ty = position.y - local_transform.b * pivot.x - local_transform.d * pivot.y;
        }
    }

    return local_transform;
}

Matrix2D Matrix2DMult(const Matrix2D curr, const Matrix2D m)
{

    Matrix2D result;

    result.a = curr.a * m.a + curr.b * m.c;
    result.b = curr.a * m.b + curr.b * m.d;
    result.c = curr.c * m.a + curr.d * m.c;
    result.d = curr.c * m.b + curr.d * m.d;

    result.tx = curr.tx * m.a + curr.ty * m.c + m.tx;
    result.ty = curr.tx * m.b + curr.ty * m.d + m.ty;

    return result;
}

Matrix2D TransformComponent::GetWorldTransformation()
{

    local_transform = GetLocalTrasformation();
    if (object->parent != nullptr)
    {
        Matrix2D mat = object->parent->transform->GetWorldTransformation();
        wordl_transform = Matrix2DMult(local_transform, mat);
        return wordl_transform;
    }
    return local_transform;
}

//*********************************************************************************************************************
//**                         Component                                                                              **
//*********************************************************************************************************************
Component::Component()
{
    object = nullptr;
    depth = 0;
}
Component::~Component()
{
    

    //  Log(LOG_INFO, "Component Destroyed");
}

//*********************************************************************************************************************
//**                         SpriteComponent                                                                              **
//*********************************************************************************************************************

SpriteComponent::SpriteComponent(const std::string &fileName) : Component()
{
    depth = 1;
    this->color = WHITE;
    FlipX = false;
    FlipY = false;
    clip.x = 0;
    clip.y = 0;
    clip.width = 1;
    clip.height = 1;
    graphID = fileName;

    graph = Assets::Instance().getGraph(fileName);
    if (graph)
    {
        clip.x = 0;
        clip.y = 0;
        clip.width = graph->width;
        clip.height = graph->height;
    }
}

void SpriteComponent::OnInit()
{
    object->centerOrigin();
    object->centerPivot();
    object->width = clip.width;
    object->height = clip.height;
}

void SpriteComponent::OnDebug()
{
    // if (object->parent)
    //     DrawText(TextFormat("%s : %s", object->name.c_str(), object->parent->name.c_str()), (int)object->getWorldX(), (int)object->getWorldY(), 5, RED);
    // else
    //     DrawText(TextFormat(" %s ", object->name.c_str()), (int)object->getWorldX(), (int)object->getWorldY(), 5, RED);

    // int x= (int)object->getWorldX() + clip.x - object->getPivotX();
    // int y= (int)object->getWorldY() + clip.y - object->getPivotY();

    //  DrawRectangleLines(x,y,clip.width,clip.height, BLUE);
}

void SpriteComponent::OnDraw()
{
    //  Log(LOG_INFO, "SpriteComponent::OnDraw");

    Matrix2D mat = object->transform->GetWorldTransformation();

    if (graph)
    {
        //  RenderTransformFlip(graph->texture, clip, FlipX, FlipY, color, &mat, 0);
        RenderTransformFlipClip(graph->texture, clip.width, clip.height, clip, FlipX, FlipY, color, &mat, 0);
    }
    else
    {

        DrawCircleLines((int)object->getX(), (int)object->getY(), 1, RED);
        //   Log(LOG_ERROR, "SpriteComponent::OnDraw  %s %f %f ",object->name.c_str() , (int)object->getX(), (int)object->getY());
    }
}

void SpriteComponent::SetClip(float x, float y, float width, float height)
{
    clip.x = x;
    clip.y = y;
    clip.width = width;
    clip.height = height;
}
void SpriteComponent::SetClip(Rectangle c)
{
    clip.x = c.x;
    clip.y = c.y;
    clip.width = c.width;
    clip.height = c.height;
}




//*********************************************************************************************************************
//**                         GameObject                                                                              **
//*********************************************************************************************************************

static unsigned long NewGameObjectID()
{
    static unsigned long id = 0;
    return id++;
}

GameObject::GameObject() : name("GameObject"),
                           alive(true), visible(true), active(true),
                           layer(1)
{
    // Log(LOG_INFO, "GameObject created");
    parent = nullptr;
    id = NewGameObjectID();
    transform = new TransformComponent(this);
    UpdateWorld();
    bound.x = 0;
    bound.y = 0;
    width = 1;
    height = 1;
    originX = 0;
    originY = 0;
    solid = false;
    prefab = false;
    persistent = false;
    collidable = true;
    pickable = false;

    word_position.x = transform->position.x;
    word_position.y = transform->position.y;

    layer = 0;
    scriptName = "";
    _x = 0;
    _y = 0;

    debugMask = 0;
}

GameObject::GameObject(const std::string &Name) : GameObject()
{

    name = Name;
}

GameObject::GameObject(const std::string &Name, int layer) : GameObject()
{
    this->layer = layer;
    name = Name;
}

void GameObject::OnReady()
{

}
void GameObject::OnPause()
{
   
}

void GameObject::OnRemove()
{
}

void GameObject::Encapsulate(float x, float y)
{
    if (bbReset)
    {
        x1 = x2 = x;
        y1 = y2 = y;
        bbReset = false;
    }
    else
    {
        if (x < x1)
            x1 = x;
        if (x > x2)
            x2 = x;
        if (y < y1)
            y1 = y;
        if (y > y2)
            y2 = y;

        bound.x = x1;
        bound.y = y1;
        bound.width = (x2 - x1);
        bound.height = (y2 - y1);
    }
}



Vector2 ApplyMatrixToPoint(const Matrix2D &matrix, float x, float y)
{
    Vector2 transformedPoint;
    transformedPoint.x = x * matrix.a + y * matrix.c + matrix.tx;
    transformedPoint.y = x * matrix.b + y * matrix.d + matrix.ty;

    return transformedPoint;
}

void GameObject::UpdateWorld()
{
    Matrix2D mat = transform->GetWorldTransformation();
    word_position = mat.TransformCoords();

    float w = width  *  transform->scale.x;
    float h = height *  transform->scale.y;
    radius = std::min(w, h) / 2.0f;

    bbReset = true;
    float newX = word_position.x;
    float newY = word_position.y;
    const auto tx1 = 0;
    const auto ty1 = 0;
    const auto tx2 = w;
    const auto ty2 = h;

    if (GetWorldAngle() != 0.0f)
    {

        const auto cost = cosf(-GetWorldAngle() * DEG2RAD);
        const auto sint = sinf(-GetWorldAngle() * DEG2RAD);

        Encapsulate(tx1 * cost - ty1 * sint + newX, tx1 * sint + ty1 * cost + newY);
        Encapsulate(tx2 * cost - ty1 * sint + newX, tx2 * sint + ty1 * cost + newY);
        Encapsulate(tx2 * cost - ty2 * sint + newX, tx2 * sint + ty2 * cost + newY);
        Encapsulate(tx1 * cost - ty2 * sint + newX, tx1 * sint + ty2 * cost + newY);
    }
    else
    {

        Encapsulate(tx1 + newX, ty1 + newY);
        Encapsulate(tx2 + newX, ty1 + newY);
        Encapsulate(tx2 + newX, ty2 + newY);
        Encapsulate(tx1 + newX, ty2 + newY);
    }

    for (auto &c : children)
    {
        c->UpdateWorld();
    }
}
void GameObject::OnCollision(GameObject *other)
{

  
    // Log(LOG_INFO, "OnColide %s with %s ", name.c_str(), other->name.c_str());
}

void GameObject::sendMensageAll()
{
    if (!scene)
        return;

    for (auto &c : scene->gameObjects)
    {
        if (c == this)
            continue;
        if (!c->alive)
            continue;
      
    }

    for (auto &c : children)
    {
        c->sendMensageAll();
    }
}

void GameObject::setDebug(int mask)
{
    debugMask = mask;
}

void GameObject::sendMensageTo(const std::string &name)
{
    if (!scene)
        return;

    for (auto &c : scene->gameObjects)
    {
        if (!c->alive)
            continue;
        if (c->name == name)
        {
                    }
    }

    for (auto &c : children)
    {
        c->sendMensageTo(name);
    }
}

void GameObject::Update(float dt)
{
    
    if (solid)
        return;

    UpdateWorld();



    for (auto &c : m_components)
    {
        c->OnUpdate(dt);
    }

    for (auto &c : children)
    {
        c->Update(dt);
    }
}

bool GameObject::place_free(float x, float y)
{
    if (!scene)
        return true;
    return scene->place_free(this, x, y);
}

bool GameObject::place_meeting(float x, float y, const std::string &name)
{
    if (!scene)
        return false;
    return scene->place_meeting(this, x, y, name);
}

bool GameObject::place_meeting_layer(float x, float y, int layer)
{

    if (!scene)
        return false;
    return scene->place_meeting_layer(this, x, y, layer);
}

bool GameObject::collideWith(GameObject *e, float x, float y)
{
    if (!scene || !e)
        return false;
    if (e == this)
        return false;

    _x = this->transform->position.x;
    _y = this->transform->position.y;
    this->transform->position.x = x;
    this->transform->position.y = y;

    if (
        x - this->getWorldOriginX() + width > e->getWorldX() - e->getWorldOriginX() &&
        y - this->getWorldOriginY() + height > e->getWorldY() - e->getWorldOriginY() &&
        x - this->getWorldOriginX() < e->getWorldX() - e->getWorldOriginX() + e->width &&
        y - this->getWorldOriginY() < e->getWorldY() - e->getWorldOriginY() + e->height)
    {
        this->transform->position.x = _x;
        this->transform->position.y = _y;
        OnCollision(e);
        e->OnCollision(this);
        return true;
    }
    this->transform->position.x = _x;
    this->transform->position.y = _y;
    return false;
}

void GameObject::centerPivot()
{

    if (HasComponent<SpriteComponent>())
    {
        auto sprite = GetComponent<SpriteComponent>();
        transform->pivot.x = sprite->clip.width / 2.0f;
        transform->pivot.y = sprite->clip.height / 2.0;
        //  Log(LOG_INFO, "Pivot %s set to %f %f", name.c_str(), transform->pivot.x, transform->pivot.y);
        // Log(LOG_INFO, "Size %s set to %f %f", name.c_str(), sprite->clip.width, sprite->clip.height);
    }
    else
    {

        transform->pivot.x = (width / 2.0f);
        transform->pivot.y = (height / 2.0f);
        // Log(LOG_INFO, "Pivot %s set to %f %f", name.c_str(), transform->pivot.x, transform->pivot.y);
    }
}

void GameObject::centerOrigin()
{

    if (HasComponent<SpriteComponent>())
    {
        auto sprite = GetComponent<SpriteComponent>();
        width = (int)sprite->clip.width / 2 * transform->scale.x;
        height = (int)sprite->clip.height / 2 * transform->scale.y;
        originX = -(int)(width / 2.0f) + (width / 2.0f);
        originY = -(int)(height / 2.0f) + (height / 2.0f);
    }
    else
    {

        originX = (width / 2.0f);
        originY = (height / 2.0f);
        // Log(LOG_INFO, "Pivot %s set to %f %f", name.c_str(), transform->pivot.x, transform->pivot.y);
    }
}

void GameObject::Debug()
{

    if (!debugMask)
        return;

    bool isOriginEnabled = (debugMask & SHOW_ORIGIN) != 0;
    bool isBoxCollideEnabled = (debugMask & SHOW_BOX) != 0;

    float finalRad = radius / 4.0f;
    if (finalRad < 0.5f)
        finalRad = 0.5f;

   

    int bX = (int)getX();
    int bY = (int)getY();

    int cx = bX + originX;
    int cy = bY + originY;
    int cw = width ;
    int ch = height ;

    if (isBoxCollideEnabled)
        DrawRectangleLines( cx,  cy, cw , ch , WHITE);
    if (isOriginEnabled)
        DrawCircle(cx, cy, finalRad, WHITE);

    if (!solid)
    {

        Vec2 p;
        if (!parent)
            p = GetLocalPoint(transform->pivot.x, transform->pivot.y);
        else
            p = GetWorldPoint(transform->pivot.x, transform->pivot.y);

        bool isPivotEnabled = (debugMask & SHOW_PIVOT) != 0;
        bool isTrasnformEnabled = (debugMask & SHOW_TRANSFORM) != 0;

        if (isPivotEnabled)
            DrawCircle((int)p.x, (int)p.y, finalRad, LIME);

        float newX = word_position.x;
        float newY = word_position.y;
        const auto tx1 = 0;
        const auto ty1 = 0;
        const auto tx2 = width  * 2.0f * transform->scale.x;
        const auto ty2 = height * 2.0f * transform->scale.y;

        if (isTrasnformEnabled)
        {
            if (GetWorldAngle() != 0.0f)
            {

                const auto cost = cosf(-GetWorldAngle() * DEG2RAD);
                const auto sint = sinf(-GetWorldAngle() * DEG2RAD);
                float x1 = tx1 * cost - ty1 * sint + newX;
                float y1 = tx1 * sint + ty1 * cost + newY;
                float x2 = tx2 * cost - ty1 * sint + newX;
                float y2 = tx2 * sint + ty1 * cost + newY;
                float x3 = tx2 * cost - ty2 * sint + newX;
                float y3 = tx2 * sint + ty2 * cost + newY;
                float x4 = tx1 * cost - ty2 * sint + newX;
                float y4 = tx1 * sint + ty2 * cost + newY;
                DrawLine(x1, y1, x2, y2, LIME);
                DrawLine(x1, y1, x4, y4, LIME);
                DrawLine(x3, y3, x4, y4, LIME);
                DrawLine(x2, y2, x3, y3, LIME);
            }
            else
            {
                DrawLine(tx1 + newX, ty1 + newY, tx2 + newX, ty1 + newY, LIME);
                DrawLine(tx1 + newX, ty1 + newY, tx1 + newX, ty2 + newY, LIME);
                DrawLine(tx2 + newX, ty2 + newY, tx1 + newX, ty2 + newY, LIME);
                DrawLine(tx2 + newX, ty2 + newY, tx2 + newX, ty1 + newY, LIME);
            }
        }
    }

    bool isComponentsEnable = (debugMask & SHOW_COMPONENTS) != 0;
    bool isBoundEnable = (debugMask & SHOW_BOUND) != 0;

    if (isBoundEnable)
        DrawRectangleLinesEx(bound, 1.5f, MAGENTA);

    if (isComponentsEnable)
    {
        for (auto &c : m_components)
        {
            c->OnDebug();
        }
    }

    for (auto &c : children)
    {
        c->Debug();
    }
}
void GameObject::Render()
{



    for (auto &c : m_components)
    {
        c->OnDraw();
    }

    
    for (auto &c : children)
    {
        c->Render();
    }

    // Log(LOG_INFO, "Render %s %f %f", name.c_str(),getX(),getY());
}

GameObject::~GameObject()
{
    // Log(LOG_INFO, "[CPP] GameObject (%s) destroyed", name.c_str());

    for (auto &c : m_components)
    {
        if (c)
        {
            c->OnDestroy();
            delete c;
        }
    }

    for (auto &c : children)
    {
        if (c)
        {
            delete c;
        }
    }
    children.clear();

}

Vec2 GameObject::GetWorldPoint(float _x, float _y)
{

    return transform->wordl_transform.TransformCoords(Vec2(_x, _y));
}

Vec2 GameObject::GetWorldPoint(Vec2 p)
{
    return transform->wordl_transform.TransformCoords(p);
}

Vec2 GameObject::GetLocalPoint(Vec2 p)
{
    return transform->local_transform.TransformCoords(p);
}

Vec2 GameObject::GetLocalPoint(float x, float y)
{
    return transform->local_transform.TransformCoords(Vec2(x, y));
}


GameObject *GameObject::addChild(GameObject *e)
{
    children.push_back(e);
    e->parent = this;
    return e;
}
