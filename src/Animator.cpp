#include "Engine.hpp"

Animator::Animator() : Component(), currentAnimation(""), nextAnimation(""),
                       isPlaying(true),
                       mode(AnimationMode::Loop)
{
    sprite = nullptr;
    object = nullptr;
    currentFrame = 0;
    frameCount = 0;
    frameDuration = 0;
    currentTime = 0;
    isReversed = false;
    isLoad = true;
}

void Animator::OnDestroy()
{
    for (auto &pair : animations)
    {
        delete pair.second;
    }
    animations.clear();
}


void Animator::OnUpdate(float deltaTime)
{
    if (animations.size() == 0 || !isLoad)
        return;

    if (!sprite)
    {
        sprite = object->GetComponent<SpriteComponent>();
        if (!sprite)
        {
            Log(LOG_ERROR, "Animator::OnUpdate() : SpriteComponent not found");
            isLoad = false;
            return;
        }
    }

    //   Log(LOG_INFO, "Animator::OnUpdate() : %s", object->name.c_str());

    if (isPlaying)
    {
        Animation *animation = GetAnimation();
        if (!animation)
            return;
        animation->Update(deltaTime, mode);

        currentFrame = animation->currentFrame;
        currentTime = animation->currentTime;
        frameCount = animation->frameCount;
        frameDuration = animation->frameDuration;
        isReversed = animation->isReversed;

        if (nextAnimation != "" && animation->currentFrame == animation->frameCount - 1)
        {
            currentAnimation = nextAnimation;
            nextAnimation = "";
            animation->currentFrame = 0;
            animation->currentTime = 0;
            Play();
        }
        // verifica se a animação atual terminou de ser reproduzida
        if (animation->currentFrame == animation->frameCount - 1)
        {
            if (mode == AnimationMode::Loop)
            {
                animation->currentFrame = 0; // reinicia a animação
                animation->currentTime = 0;
            }
            else if (mode == AnimationMode::PingPong)
            {
                animation->isReversed = true; // inverte a reprodução da animação
            }
        }

        // verifica se a animação atual terminou de ser reproduzida no modo PingPong
        if (animation->currentFrame == 0 && animation->isReversed)
        {
            if (mode == AnimationMode::PingPong)
            {
                animation->isReversed = false; // inverte a reprodução da animação de volta
            }
        }
    }

    Animation *animation = GetAnimation();
    if (!animation)
        return;

    if (sprite)
    {

        Rectangle frameRectangle = animation->GetFrame();
        sprite->clip.x = frameRectangle.x;
        sprite->clip.y = frameRectangle.y;
        sprite->clip.width = frameRectangle.width;
        sprite->clip.height = frameRectangle.height;
        sprite->graph = animation->graph;
    }
}

void Animator::Play()
{
    isPlaying = true;
}

void Animator::Pause()
{
    isPlaying = false;
}

void Animator::Stop()
{
    isPlaying = false;
    if (animations.size() == 0 || !isLoad)
        return;
    Animation *animation = GetAnimation();
    if (!animation)
        return;

    animation->currentFrame = 0;
    animation->currentTime = 0;
}

void Animator::SetAnimation(const std::string &name, bool now)
{
    if (animations.size() == 0 || !isLoad)
        return;
    if (now)
    {
        currentAnimation = name;
        nextAnimation = "";
        Play();
    }
    else if (currentAnimation != name)
    {
        if (nextAnimation == "")
        {
            nextAnimation = name; // marca a próxima animação a ser reproduzida
        }
        else
        {
            nextAnimation = name; // sobrescreve a próxima animação marcada
        }
    }
}
Animation *Animator::GetAnimation()
{
    auto it = std::find_if(animations.begin(), animations.end(),
                           [this](const std::pair<std::string, Animation *> &pair)
                           {
                               return pair.first == currentAnimation;
                           });
    if (it != animations.end())
    {
        return it->second;
    }
    else
    {

        return animations.front().second;
    }
}

void Animator::AddAnimation(const std::string &name, Animation *animation)
{
    animations.push_back(std::make_pair(name, animation));
    if (currentAnimation == "")
    {
        currentAnimation = name;
    }
    OnUpdate(0);
}

void Animator::Add(const std::string &name, const std::string &graph, int rows, int columns, int frameCount, float framesPerSecond)
{
    float frameDuration = 1.0f / framesPerSecond;
    Animation *animation = new Animation(graph, rows, columns, frameCount, frameDuration);
    AddAnimation(name, animation);
}

void Animator::SetMode(AnimationMode mode)
{
    this->mode = mode;
    if (animations.size() == 0 || !isLoad)
        return;
    Animation *animation = GetAnimation();
    if (!animation)
        return;
    animation->currentFrame = 0;
    animation->currentTime = 0;
    animation->isReversed = false;
}
void Animator::OnInit()
{
    if (object != nullptr)
    {
        if (object->HasComponent<SpriteComponent>())
        {
            sprite = object->GetComponent<SpriteComponent>();
        }
        else
        {
            Log(LOG_ERROR, "SpriteComponent::OnInit() : GameObject has no SpriteComponent");
            isLoad = false;
        }
    }
}

void Animator::OnDebug()
{
    // Log(LOG_INFO, "Animator::OnDebug()");
}



//*********************************************************************************************************************
//**                         ANIMATION                                                                              **
//*********************************************************************************************************************

Animation::Animation(const std::string &graphID, int rows, int columns, int frameCount, float frameDuration) : frameCount(frameCount),
                                                                                                               currentFrame(0),
                                                                                                               frameDuration(frameDuration),
                                                                                                               currentTime(0),
                                                                                                               isReversed(false), rows(rows),
                                                                                                               columns(columns)

{
    this->graphID = graphID;
    graph = Assets::Instance().getGraph(graphID);
    if (graph)
    {
        imageWidth = graph->texture.width;
        imageHeight = graph->texture.height;
        // Log(LOG_INFO, "Animation::Animation() : graphID %s %d %d", graphID.c_str(), imageWidth, imageHeight);
    }
    else
    {
        Log(LOG_ERROR, "Animation::Animation() : graphID not found %s", graphID.c_str());
    }
}

Rectangle Animation::GetFrame()
{
    Rectangle rect;
    if (graph == nullptr)
    {
        Log(LOG_ERROR, "Animation::GetFrame() : graph is null");
        return rect;
    }
    rect.width = imageWidth / columns;
    rect.height = imageHeight / rows;
    rect.x = (currentFrame % columns) * rect.width;
    rect.y = (currentFrame / columns) * rect.height;
    return rect;
}

void Animation::Update(float deltaTime, AnimationMode mode)
{
    if (!graph)
        return;
    currentTime += deltaTime;

    if (mode == AnimationMode::Loop)
    {
        while (currentTime >= frameDuration)
        {
            currentFrame = (currentFrame + 1) % frameCount;
            currentTime -= frameDuration;
        }
    }
    else if (mode == AnimationMode::PingPong)
    {
        int frameIndex = currentFrame;
        int frameIncrement = isReversed ? -1 : 1;

        while (currentTime >= frameDuration)
        {
            frameIndex += frameIncrement;

            if (frameIndex < 0)
            {
                frameIndex = 1;
                isReversed = false;
            }
            else if (frameIndex >= frameCount)
            {
                frameIndex = frameCount - 2;
                isReversed = true;
            }

            currentTime -= frameDuration;
        }

        currentFrame = frameIndex;
    }
    else if (mode == AnimationMode::Stop)
    {
        if (currentFrame < frameCount - 1)
        {
            while (currentTime >= frameDuration)
            {
                currentFrame++;
                currentTime -= frameDuration;
            }
        }
    }
    else if (mode == AnimationMode::Once)
    {
        if (currentFrame < frameCount - 1)
        {
            while (currentTime >= frameDuration)
            {
                currentFrame++;
                currentTime -= frameDuration;
            }
        }
    }
}