#include "ofxPublishScreen.h"

//#define DEFINE_DATATYPE 1

#pragma mark - Publisher

class ofxPublishScreen::Publisher::Thread : public ofThread
{
public:
    
    //Define Image and json data as struct
    class ofxPublishScreenData {
    public:
    
        //ofxPublishScreenData(const ofPixels &_pix) : pix(_pix){;}
        //const ofPixels &pix;
        ofPixels pix;
        ofxJSONElement json;
    } ;
    
    
    
	Thread(string host, ofImageFormat format) : format(format), last_pubs_time(0), pubs_fps(0), compress_time_ms(0)
	{
		pub.setHighWaterMark(1);
		pub.bind(host);
	}

	void pushImage(const ofPixels &pix, const ofxJSONElement &json)
	{
		if (lock())
		{
            
            
#ifndef DEFINE_DATATYPE
			frames.push(pix);
			unlock();
#else

            ofxPublishScreenData data;
            data.pix = pix;             //copy happen? //TODO
            data.json = json;
			frames.push(data);
			unlock();

#endif
            
            
            

		}
	}

	float getFps() { return pubs_fps; }
	
protected:
	
	ofxZmqPublisher pub;
    
    //TODO this should be modifyed
    
#ifndef DEFINE_DATATYPE
	queue<ofPixels, list<ofPixels> > frames;
#else
    queue<ofxPublishScreenData, list<ofxPublishScreenData> > frames;
#endif
	ofImageFormat format;
	ofxTurboJpeg jpeg;
	
	float pubs_fps;
	float last_pubs_time;
	
	float compress_time_ms;
	
	void threadedFunction()
	{
		while (isThreadRunning())
		{
			while (!frames.empty() && isThreadRunning())
			{
				ofPixels pix;
                ofxJSONElement json;

				if (lock())
				{
#ifndef DEFINE_DATATYPE
					pix = frames.front();
                    
#else
                    json = frames.front().json;
					pix = frames.front().pix;
#endif
                    
					frames.pop();
					unlock();
				}

				ofBuffer data;

				{
					float comp_start = ofGetElapsedTimeMillis();
					jpeg.save(data, pix,  75);
					float d = ofGetElapsedTimeMillis() - comp_start;
					compress_time_ms += (d - compress_time_ms) * 0.1;
				}
				
                //added by shks
                {
                    ofBuffer json_data;
                    json["test"] = "test data";
                    json_data.set(json.toStyledString());
                    
                    data.append("\nEMBEDDED_INFORMATION\n");
                    data.append("\n");
                    data.append(json_data);
                    data.append("\n");
                }
                
				pub.send(data, true);
				
				float t = ofGetElapsedTimef();
				float d = t - last_pubs_time;
				d = 1. / d;
				
				pubs_fps += (d - pubs_fps) * 0.1;
				last_pubs_time = t;
			}

			ofSleepMillis(1);
		}
	}
};

void ofxPublishScreen::Publisher::setup(int port, ofImageFormat format)
{
	dispose();

	char buf[256];
	sprintf(buf, "tcp://*:%i", port);

	thread = new Thread(buf, format);
	thread->startThread();
	
	ofAddListener(ofEvents().exit, this, &Publisher::onExit);
}

void ofxPublishScreen::Publisher::dispose()
{
	if (thread)
	{
		Thread *t = thread;
		thread = NULL;
		t->stopThread();
		delete t;
	}
}

ofxPublishScreen::Publisher::~Publisher()
{
	dispose();
}

void ofxPublishScreen::Publisher::publishScreen()
{
	ofxJSONElement json;
    json["test"] = "test";
    
    publishScreen(json);
}


void ofxPublishScreen::Publisher::publishScreen(ofxJSONElement json)
{
	int w = ofGetWidth();
	int h = ofGetHeight();

	ofTexture tex;
	tex.allocate(w, h, GL_RGBA);
	tex.loadScreenData(0, 0, w, h);

	publishTexture(&tex, json);

	tex.clear();
}

void ofxPublishScreen::Publisher::publishPixels(const ofPixels &pix, ofxJSONElement json)
{
    //TODO define some strut for image and json data
	thread->pushImage(pix, json );
}

void ofxPublishScreen::Publisher::publishTexture(ofTexture* inputTexture, ofxJSONElement json)
{
	ofPixels pix;
	inputTexture->readToPixels(pix);
	publishPixels(pix, json);
}

void ofxPublishScreen::Publisher::onExit(ofEventArgs&)
{
	dispose();
}

float ofxPublishScreen::Publisher::getFps()
{
	return thread->getFps();
}

#pragma mark - Subscriber

class ofxPublishScreen::Subscriber::Thread : public ofThread
{
public:
	
	ofxZmqSubscriber subs;
	ofPixels pix;
	ofxTurboJpeg jpeg;
    ofxJSONElement receivedJson;
    
	
	bool is_frame_new;
	float last_subs_time;
	float subs_fps;
	
	Thread(string host) : is_frame_new(false), last_subs_time(0), subs_fps(0)
	{
		subs.setHighWaterMark(2);
		subs.connect(host);
	}

	void threadedFunction()
	{
		while (isThreadRunning())
		{
			while (subs.hasWaitingMessage())
			{
				ofBuffer data;
				subs.getNextMessage(data);

				ofPixels temp;
				if (jpeg.load(data, temp))
				{
                    //Image data process
					if (lock())
					{
						pix = temp;
						is_frame_new = true;
						unlock();
						
						float d = ofGetElapsedTimef() - last_subs_time;
						d = 1. / d;
						
						subs_fps += (d - subs_fps) * 0.1;
						last_subs_time = ofGetElapsedTimef();
					}
                    
                    // added by shks - - - - - - - - - - - - - - - - -
                    //Json data Process
                    {
                        string str_received;
                        bool json_decodeMode = false;
                        while (!data.isLastLine()) {
                            
                            string str = data.getNextLine();
                            
                            if(json_decodeMode)
                            {
                                str_received.append(str);
                            }
                            
                            if( str == "EMBEDDED_INFORMATION" )
                            {
                                json_decodeMode = true;
                            }
                        }
                        
                        //receivedJson
                        if (lock())
                        {
                            receivedJson = str_received;
                            unlock();
                        }
                    }
                    // added by shks - - - - - - - - - - - - - - - - -
				}
			}
			
			ofSleepMillis(1);
		}
	}
	
	float getFps()
	{
		return subs_fps;
	}

};

void ofxPublishScreen::Subscriber::setup(string host, int port)
{
	dispose();

	char buf[256];
	sprintf(buf, "tcp://%s:%i", host.c_str(), port);
	
	thread = new Thread(buf);
	thread->startThread();
}

void ofxPublishScreen::Subscriber::dispose()
{
	if (thread)
	{
		Thread *t = thread;
		thread = NULL;
		t->stopThread();
		delete t;
	}
}

void ofxPublishScreen::Subscriber::update()
{
	is_frame_new = false;
	
	if (thread->lock())
	{
		if (thread->is_frame_new)
		{
			thread->is_frame_new = false;
			pix = thread->pix;
			receivedJson = thread->receivedJson;        //Added by shks
            
			is_frame_new = true;
		}
		thread->unlock();
	}
}

float ofxPublishScreen::Subscriber::getFps()
{
	return thread->getFps();	
}
