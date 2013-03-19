#include "testApp.h"
#include "ofxPublishScreen.h"

ofxPublishScreen::Subscriber subs;
//--------------------------------------------------------------
void testApp::setup(){	
	// initialize the accelerometer
	ofxAccelerometer.setup();
	
	//If you want a landscape oreintation 
	//iPhoneSetOrientation(OFXIPHONE_ORIENTATION_LANDSCAPE_RIGHT);
	
	ofBackground(127,127,127);
	subs.setup("127.0.0.1", 20000);

}

//--------------------------------------------------------------
void testApp::update(){
	subs.update();
	image.setFromPixels(
						subs.getPixelsRef().getPixels() ,
						subs.getPixelsRef().getWidth(),
						subs.getPixelsRef().getHeight(),
						OF_IMAGE_COLOR);
}

//--------------------------------------------------------------
void testApp::draw(){
	image.draw(0,0);
	
}

//--------------------------------------------------------------
void testApp::exit(){

}

//--------------------------------------------------------------
void testApp::touchDown(ofTouchEventArgs & touch){

}

//--------------------------------------------------------------
void testApp::touchMoved(ofTouchEventArgs & touch){

}

//--------------------------------------------------------------
void testApp::touchUp(ofTouchEventArgs & touch){

}

//--------------------------------------------------------------
void testApp::touchDoubleTap(ofTouchEventArgs & touch){

}

//--------------------------------------------------------------
void testApp::touchCancelled(ofTouchEventArgs & touch){
    
}

//--------------------------------------------------------------
void testApp::lostFocus(){

}

//--------------------------------------------------------------
void testApp::gotFocus(){

}

//--------------------------------------------------------------
void testApp::gotMemoryWarning(){

}

//--------------------------------------------------------------
void testApp::deviceOrientationChanged(int newOrientation){

}

