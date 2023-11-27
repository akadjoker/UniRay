#include "Engine.hpp"
#include "Scene.hpp"



int screenWidth = 1020;
int screenHeight = 750;

Scene scene;

// void *operator new(std::size_t size)
// {
//   allocatedMemory += size;
//   return std::malloc(size);
// }

// void operator delete(void *ptr, std::size_t size) noexcept
// {
//   allocatedMemory -= size;
//   std::free(ptr);
// }

void testeBemshmark()
{
Assets::Instance().loadGraph("wabbit", "assets/wabbit_alpha.png");

   GameObject *gamePlayer = new GameObject("scene");
   gamePlayer->persistent=true;
   scene.AddGameObject(gamePlayer);


  GameObject *wabbit = new GameObject("wabbit");
  SpriteComponent *spr = wabbit->AddComponent<SpriteComponent>("wabbit");
  wabbit->persistent=true;
  scene.AddGameObject(wabbit);
}

void testeShooter()
{

 Assets::Instance().loadGraph("nave", "assets/playerShip1_orange.png");
  Assets::Instance().loadGraph("wabbit", "assets/wabbit_alpha.png");
  Assets::Instance().loadGraph("bala", "assets/texture.png");
  Assets::Instance().loadGraph("lazer", "assets/laserRed08.png");
  Assets::Instance().loadGraph("cano", "assets/gun02.png");
  Assets::Instance().loadGraph("ufo", "assets/ufoRed.png");


  GameObject *gamePlayer = new GameObject("player");
  gamePlayer->transform->position.x = 400;
  gamePlayer->transform->position.y = 400;
  gamePlayer->AddComponent<SpriteComponent>("nave");
  scene.AddGameObject(gamePlayer);

  {
    GameObject *gameCano = new GameObject("cano");
    gameCano->AddComponent<SpriteComponent>("cano");
  
    gameCano->transform->position.x = 20;
    gameCano->transform->position.y = 30;
    gamePlayer->addChild(gameCano);
  }
  {
    GameObject *gameCano = new GameObject("cano");
    gameCano->AddComponent<SpriteComponent>("cano");
  
    gameCano->transform->position.x = 80;
    gameCano->transform->position.y = 30;
    gamePlayer->addChild(gameCano);
  }

  {
    GameObject *gameOvni = new GameObject("ufo");
    gameOvni->AddComponent<SpriteComponent>("ufo");

    gameOvni->transform->position.x = 200;
    gameOvni->transform->position.y = 200;
    scene.AddGameObject(gameOvni);
  }

    {
    GameObject *gameOvni = new GameObject("ufo");
    gameOvni->AddComponent<SpriteComponent>("ufo");

    gameOvni->transform->position.x = 700;
    gameOvni->transform->position.y = 200;
    scene.AddGameObject(gameOvni);
  }

}



void testeMovements()
{
  Assets::Instance().loadGraph("wabbit", "assets/wabbit_alpha.png");
  Assets::Instance().loadGraph("player_run", "assets/Player/Run.png");
  Assets::Instance().loadGraph("player_jump", "assets/Player/Jump.png");
  Assets::Instance().loadGraph("player_die", "assets/Player/Die.png");
  Assets::Instance().loadGraph("player_idle", "assets/Player/Idle.png");
  Assets::Instance().loadGraph("player_win", "assets/Player/Celebrate.png");


   GameObject *gamePlayer = new GameObject("main");

   gamePlayer->persistent=true;
   scene.AddGameObject(gamePlayer);


  GameObject *wabbit = new GameObject("player");

  wabbit->AddComponent<SpriteComponent>("player_idle");
  Animator *animation = wabbit->AddComponent<Animator>();
  animation->Add("run", "player_run",1,10, 10,12);
  animation->Add("win", "player_win",1,10, 11,10);
  animation->Add("jump", "player_jump",1,11, 11,8);
  animation->Add("idle", "player_idle",1,1, 1,1);
  animation->SetAnimation("run");
  animation->Play();
  wabbit->transform->position.x = 300;
  wabbit->transform->position.y = 400;
  //animation->SetMode(AnimationMode::Once);

  
  scene.AddGameObject(wabbit);
}

int main()
{



  InitWindow(screenWidth, screenHeight, "2D Engine");
  


  scene.Init("Moves",60,screenWidth, screenHeight, false);
  scene.SetBackground(0,0,0);
  scene.SetWorld(screenWidth, screenHeight);

  testeShooter();
  scene.enableEditor = true;

  //scene.Load("assets/scene.json");

//SetWindowSize((int)scene.windowSize.x, (int)scene.windowSize.y);
SetWindowTitle(scene.title.c_str());
SetTargetFPS(scene.fps);



rlSetFramebufferWidth(scene.windowSize.x);
rlSetFramebufferHeight(scene.windowSize.y);


testeMovements();
//testeBemshmark();
//testeShooter();





  while (!WindowShouldClose())
  {



    BeginDrawing();

    scene.Update();

     scene.Render();

    EndDrawing();
  }

 // scene.Save("assets/tmp.json");

  scene.ClearAndFree();
  
  CloseWindow();

  return 0;
}