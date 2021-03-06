#ifndef THREADPOOL_HEADER
#define THREADPOOL_HEADER
// a thread pool using polymorphism and a thread-safe queue
// there are some pretty big design issues with this code
// what are they?

#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <vector>
#include <string>

#include "message_queue.h"
#include "objects.hpp"
#include "plane.hpp"

// define a base class to encapsulate a unit of work
class WorkBase
{
public:
	WorkBase(std::vector< std::vector<ray*> > * pic, std::list<objects*> * act, std::list<light*> * lig, plane * scr, int rowIn, int widthIn);
	~WorkBase();
	void run();
	int getRow();
	double getMaxColor();
	std::vector<colorStruct*> * getRowColors();

private:
	std::vector< std::vector<ray*> >  * picture;
	std::list<objects*> * actors;
	std::list<light*> * lights;
	plane * screen = 0;
	double maxValue = -1;
	std::vector<colorStruct*> rowColors;
	int	row = -1;
	int width = -1;
};

WorkBase::WorkBase(std::vector<std::vector<ray*>> * pic, std::list<objects*> * act, std::list<light*> * lig, plane * scr, int rowIn, int widthIn)
{
	picture = pic;
	actors = act;
	lights = lig;
	screen = scr;
	row = rowIn;
	width = widthIn;
}

inline WorkBase::~WorkBase()
{
	for (int i = 0; i < rowColors.size(); i++)
	{
		delete rowColors[i];
	}
	//std::cout << "work dest\n";
	rowColors.clear();
}

void WorkBase::run()
{
	colorStruct * colorFromActor;
	double dist = -1;
	double minDist = -1;
	for(int x = 0; x  < width; x ++)
	{
		//std::cout << x << " of width " << width << '\n';
		ray * castRay = (*picture)[row][x];
		objects * actor = 0;
		minDist = -1;
		for (auto const& obj : *actors) {
			
			dist = obj->intersectTrue(*castRay);
			//std::cout << dist << ' ' << minDist << '\n';
			if (dist < 0 || (minDist > 0 && minDist < dist))
			{
				continue;
			}
			else
			{
				actor = obj;
				minDist = dist;
			}
		}
		if (minDist > 0)
		{
			
			//std::cout << actor->getCenter()->x << '\n';
			//std::cout << i << " " << j << '\n';
	
			colorFromActor = actor->intersect(*actors, *lights, *castRay, minDist, screen);
			//std::cout << "after110\n";
			if (colorFromActor->r > maxValue)
			{
				maxValue = colorFromActor->r;
			}
			if (colorFromActor->g > maxValue)
			{
				maxValue = colorFromActor->g;
			}
			if (colorFromActor->b > maxValue)
			{
				maxValue = colorFromActor->b;
			}
			rowColors.push_back(colorFromActor);
		}
		else
		{
			rowColors.push_back(new colorStruct(0, 0, 0));
		}
	}

}

inline int WorkBase::getRow()
{
	return row;
}

double WorkBase::getMaxColor()
{
	return maxValue;
}

inline std::vector<colorStruct*>* WorkBase::getRowColors()
{
	return &rowColors;
}




// make the message a WorkBase pointer
typedef WorkBase * MessageType;
typedef message_queue<MessageType> MessageQueue;

// a thread function to remove work and run it
// exits on a nullptr message
void thread_func(MessageQueue * inq, MessageQueue * outq)
{
  while(true){

    // take a unit fo work from the input queue
    MessageType m;
    inq->wait_and_pop(m);

	if (m == nullptr) 
	{ 
		break;
	}
    
    // execute m
    m->run();

    // put the result back into the output queue
    outq->push(m);
  }
}

class ThreadPool
{
public:

  ThreadPool(int n, MessageQueue * input_queue, MessageQueue * output_queue){
    
    for(int i = 0; i < n;++i){
      pool.emplace_back(std::thread(&thread_func, input_queue, output_queue));
    }
  }

  void joinAll(){
    for(auto &t : pool) t.join();
  }
  
private:

  std::vector<std::thread> pool;
};

#endif
