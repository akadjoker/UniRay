#include "Engine.hpp"
#include "Scene.hpp"
#include <string>
#include <sstream>
TileLayerComponent::TileLayerComponent(int width, int height, int tileWidth, int tileHeight, int spacing, int margin, const std::string &fileName) : tileWidth(tileWidth), tileHeight(tileHeight), spacing(spacing), margin(margin), width(width), height(height)
{

    graph = Assets::Instance().getGraph(fileName.c_str());
    if (!graph)
    {
        Log(LOG_ERROR, "TileLayerComponent::TileLayerComponent  %s ", fileName.c_str());
        isLoad = false;
    }
    isLoad = true;
    graphID = fileName;
    worldWidth = width * tileWidth;
    worldHeight = height * tileHeight;
    // tileMap.reserve(width * height + 1);
    for (int i = 0; i < width * height; i++)
    {
        tileMap.push_back(-1);
    }
}


void TileLayerComponent::PaintRectangle(int x, int y, int w, int h, int id)
{
    if (!isLoad || !isWithinBounds(x, y))
        return;
    for (int i = x; i < x + w; ++i)
    {
        for (int j = y; j < y + h; ++j)
        {
            setTile(i, j, id);
        }
    }
}

void TileLayerComponent::PaintCircle(int x, int y, int radius, int id)
{
    if (!isLoad || !isWithinBounds(x, y))
        return;

    int rsq = radius * radius;
    for (int i = x - radius; i <= x + radius; ++i)
    {
        for (int j = y - radius; j <= y + radius; ++j)
        {
            int dx = i - x;
            int dy = j - y;
            if (dx * dx + dy * dy <= rsq)
            {
                setTile(i, j, id);
            }
        }
    }
}
void TileLayerComponent::OnInit()
{
            object->originX=0;
            object->originY=0;
            object->width=width*tileWidth;
            object->height=height*tileHeight;
            object->bound.x=0;
            object->bound.y=0;
            object->bound.width=width*tileWidth;
            object->bound.height=height*tileHeight;
           // object->setDebug( SHOW_BOX |  SHOW_BOUND );

}

void TileLayerComponent::OnDebug()
{
    Log(LOG_INFO, "TileLayerComponent::OnDebug");

    if (!isLoad)
        return;

    DrawRectangle(0,0,width*tileWidth,height*tileHeight, RED);
}

void TileLayerComponent::OnDraw()
{
    //  Log(LOG_INFO, "TileLayerComponent::OnDraw");
    if (!isLoad || !graph)
        return;
    if (width == 0 || height == 0 || tileWidth == 0 || tileHeight == 0)
    {
        Log(LOG_ERROR, "TileLayerComponent::OnDraw %d %d  %d %d", width, height, tileWidth, tileHeight);
        isLoad = false;
        return;
    }

    Scene *scene = Scene::Instance();


    // auto startTime = std::chrono::high_resolution_clock::now();

    


//loop in view
float zoom = scene->camera.zoom;
Vector2 offset = scene->camera.offset;
Vector2 target = scene->camera.target;
Rectangle cameraView = {
    -offset.x/zoom + target.x - (scene->windowSize.x/2.0f/zoom),
    -offset.y/zoom + target.y - (scene->windowSize.y/2.0f/zoom),
    (float)scene->windowSize.x/zoom + (offset.x/zoom),
    (float)scene->windowSize.y/zoom + (offset.y/zoom)
};

int startX = (int)(cameraView.x / tileWidth);
int startY = (int)(cameraView.y / tileHeight);
int endX = (int)((cameraView.x + cameraView.width) / tileWidth) + 1;
int endY = (int)((cameraView.y + cameraView.height) / tileHeight) + 1;

    startX = Clamp(startX, 0, width);
    startY = Clamp(startY, 0, height);
    endX = Clamp(endX, 0, width);
    endY = Clamp(endY, 0, height);

    for (int i = startY; i < endY; i++)
    {
        for (int j = startX; j < endX; j++)
        {
            float posX = (float)(j * tileWidth);
            float posY = (float)(i * tileHeight);
            Rectangle tileRect = {posX, posY, tileWidth, tileHeight};
            if (!scene->inView(tileRect))
                    continue;

            int tile = getTile(j, i);
            if (tile != -1)
            {

                RenderTile(graph->texture,
                           posX, posY,
                           tileWidth, tileHeight,
                           getClip(tile),
                           false, false, 0);
                    
            }
            // 736
        }
    }
//  auto endTime = std::chrono::high_resolution_clock::now();
// auto deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count() / 1000000.0f;
   
//     Log(LOG_INFO, "loop in view %f",deltaTime);


// //loop all

//   startTime = std::chrono::high_resolution_clock::now();


//     for (int i = 0; i < height; i++)
//     {
//         for (int j = 0; j < width; j++)
//         {
//                float  posX=(float)(j * tileWidth );
//                float  posY=(float)(i * tileHeight);
//                Rectangle tileRect = {posX, posY, tileWidth, tileHeight};
//                if (!scene->inView(tileRect))
//                     continue;
                
                

//                      int tile= getTile(j,i) ;
//                      if (tile!=-1)
//                      {

//                         RenderTile(graph->texture,
//                                     posX,posY,
//                                     tileWidth,tileHeight,
//                                     getClip(tile),
//                                     false,false,0);
                        
//                     }
//                     //115 ;)

//         }
//     }
//  endTime = std::chrono::high_resolution_clock::now();
//  deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count() / 1000000.0f;

//     Log(LOG_INFO, "loop all %f",deltaTime);




  

    //  //   Log(LOG_INFO, "TileLayerComponent::OnDraw %d %d  %d %d", width, height,tileWidth, tileHeight);
}

void TileLayerComponent::loadFromArray(const int *tiles)
{
    tileMap.clear();
    int arraSize = (tileWidth * tileHeight);
    for (int i = 0; i < arraSize; i++)
    {
        tileMap.push_back(tiles[i]);
    }
}
void TileLayerComponent::loadFromCSVFile(const std::string &filename)
{
    //    m_tileMap.clear();
    //    std::string tmp;
    //    char delim = ','; // Ddefine the delimiter to split by
    //    std::ifstream myFile(filename);
    //    std::getline(myFile,tmp,delim);
    //    while (std::getline(myFile,tmp,delim))
    //   {
    //      int index = std::stoi(tmp);
    //      m_tileMap.push_back(index);
    //    }

    if (!FileInPath(filename))
    {
        Log(LOG_ERROR, "The file  %s dont exists ", filename.c_str());
        return;
    }
    std::string path = GetPath(filename);
    char *text = LoadFileText(path.c_str());

    if (text == nullptr)
    {
        Log(LOG_ERROR, " Reading  %s", filename.c_str());
        return;
    }

    std::istringstream file(text);
    tileMap.clear();
    tileMap.reserve(width * height);

    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream lineStream(line);
        std::string cell;
        while (std::getline(lineStream, cell, ','))
        {
            int tile = std::stoi(cell);
            tileMap.push_back(tile);
        }
    }

    UnloadFileText(text);
}

void TileLayerComponent::loadFromString(const std::string &text,int shift)
{
    std::istringstream file(text);
    tileMap.clear();
    tileMap.reserve(width * height);

    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream lineStream(line);
        std::string cell;
        while (std::getline(lineStream, cell, ','))
        {
            int tile = std::stoi(cell) + shift;
            tileMap.push_back(tile);
        }
    }
}

std::string TileLayerComponent::getCSV() const
{
    std::ostringstream csvStream;
    for (size_t i = 0; i < tileMap.size(); ++i)
    {
        csvStream << tileMap[i];

        if (i < tileMap.size() - 1)
        {
            csvStream << ",";
        }
    }
    return csvStream.str();
}

void TileLayerComponent::saveToCSVFile(const std::string &filename)
{
    std::ostringstream file;

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            int tile = tileMap[y * width + x];
            file << tile;
            if (x < width - 1)
            {
                file << ",";
            }
        }
        file << "\n";
    }

    std::string fileContent = file.str();

    bool success = SaveFileText(filename.c_str(), const_cast<char *>(fileContent.c_str()));

    if (!success)
    {
        Log(LOG_ERROR, "Saving file: %s", filename.c_str());
    }
}

void TileLayerComponent::setTile(int x, int y, int tile)
{
    if (!isLoad || !isWithinBounds(x, y))
        return;

    int index = (int)(x + y * width);
    tileMap[index] = tile;
}
int TileLayerComponent::getTile(int x, int y)
{
    if (!isLoad || !isWithinBounds(x, y))
        return -1;
    int index = (int)(x + y * width);
    return tileMap[index];
}

void TileLayerComponent::createSolids()
{
     int count=0;
     for (int x = 0; x < width; x++)
        {
            for (int y = 0; y < height; y++)
            {
                int tile= getTile(x,y) ;
                if (tile!=-1)
                {
                    if (tile>=1)
                    {

                     GameObject *solid = new GameObject("solid",2);
                    solid->solid=true;
                    solid->prefab=true;
                    solid->transform->position.x=x*tileWidth,
                    solid->transform->position.y=y*tileHeight;

                    solid->width = tileWidth;
                    solid->height= tileHeight;

                    solid->originX=0;// (tileWidth/2) + (tileWidth/2)  +14;
                    solid->originY=0;// (tileHeight/2) + (tileHeight/2)+14;
                    solid->transform->pivot.x=0;
                    solid->transform->pivot.y=0;

                    solid->UpdateWorld();
              

     

                    
//                    solid->setDebug( SHOW_BOX );

                    Scene::Instance()->AddGameObject(solid);

                    }

                    count++;
                }
            }
        }
        
}

Rectangle TileLayerComponent::getClip(int id)
{
    Rectangle clip;
    if (!graph || !isLoad)
    {
        Log(LOG_ERROR, "TileLayerComponent::getClip - Graph is null");
        return clip;
    }

    int columns = (int)floor(graph->width / tileWidth);
    // int rows = (int)floor(graph->height / tileHeight);

    int pRow = id / columns;
    int pFrame = id % columns;

    float sourcex = margin + (spacing + tileWidth) * pFrame;
    float sourcey = margin + (spacing + tileHeight) * pRow;

    clip.x = sourcex;
    clip.y = sourcey;
    clip.width = tileWidth;
    clip.height = tileHeight;

    return clip;
}

void TileLayerComponent::clear()
{
    tileMap.clear();
}

void TileLayerComponent::addTile(int index)
{
    tileMap.push_back(index);
}
