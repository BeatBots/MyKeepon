#include "testApp.h"

#define PANEL_WIDTH 100

//--------------------------------------------------------------
testApp::testApp() :
mainGui(0,0,ofGetWidth(),ofGetHeight()/20),
ofBaseApp(){ }

//--------------------------------------------------------------
void testApp::setup(){
	ofSetVerticalSync(true);
	ofBackgroundHex(0x00);

	////////// MAIN GUI
	mainGui.setFont("verdana.ttf");
	mainGui.addWidgetDown(new ofxUILabel("MyKeepon Controller", OFX_UI_FONT_MEDIUM));
	mainGui.addSpacer(mainGui.getRect()->width,4);
	mainGui.addWidgetDown(new ofxUILabelButton("Add a Keepon", false));
	mainGui.setColorBack(ofColor(100,200));
	mainGui.autoSizeToFitWidgets();
	ofAddListener(mainGui.newGUIEvent,this,&testApp::mainGuiEvent);
}

//--------------------------------------------------------------
void testApp::update(){
	bool adjustNeeded = false;

	// update all panels, check if any have to be removed
	for(int i=thePanels.size()-1; i>-1; --i){
		MyKeeponControlPanel* mp = thePanels.at(i);
		if(mp->toDelete()){
			delete mp;
			thePanels.erase(thePanels.begin()+i);
			adjustNeeded = true;
		}
		else{
			mp->update();
		}
	}
	// adjust panels if needed
	if(adjustNeeded){
		adjustPanels();
	}

	// for DEBUG
	if(ofGetFrameNum()%100 == 0){
		cout << ofGetFrameRate() << endl;
	}
}

//--------------------------------------------------------------
void testApp::draw(){

}

//--------------------------------------------------------------
void testApp::mainGuiEvent(ofxUIEventArgs &e){
	string name = e.widget->getName();
	if((name.compare("Add a Keepon") == 0) && (((ofxUIButton*)e.widget)->getValue())){
		// only important to get correct y position for the panels
		//      the x position is adjusted later
		MyKeeponControlPanel* mp = new MyKeeponControlPanel(ofVec2f(0,mainGui.getRect()->y+mainGui.getRect()->height+10));
		thePanels.push_back(mp);
		adjustPanels();
	}
}

//--------------------------------------------------------------
void testApp::adjustPanels(){
	for(int i=0, xPos=0; i<thePanels.size(); ++i){
		MyKeeponControlPanel* mp = thePanels.at(i);
		mp->setX(xPos);
		xPos += mp->getRectangle().width + 10;
	}
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){

}

//--------------------------------------------------------------
void testApp::keyReleased(int key){

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){

}

//--------------------------------------------------------------
// doesn't exclude empty words
string testApp::fitStringToWidth(const string s, const int w, ofTrueTypeFont ttf){
	string retStr="";
	istringstream ss( s );
	while (!ss.eof()){
		string word;
		getline(ss,word,' ');
		if(ttf.stringWidth(retStr+" "+word) > w){
			retStr += "\n";
		}
		else if(retStr.size() != 0){
			retStr += " ";
		}
		retStr += word;
	}
	return retStr;
}
