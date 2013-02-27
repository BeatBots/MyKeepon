#include "MyKeeponControlPanel.h"

vector<string> MyKeeponControlPanel::theSerials = vector<string>();

vector<string>& MyKeeponControlPanel::updateSerialList(){
	theSerials.clear();
	theSerials.push_back("Refresh List");
	
	ofSerial tSerial;
	vector<ofSerialDeviceInfo> serialList = tSerial.getDeviceList();
	for(int i=0; i<serialList.size(); i++){
		string thisDevicePath = serialList.at(i).getDevicePath();
		theSerials.push_back(thisDevicePath);
	}
	return theSerials;
}

MyKeeponControlPanel::GazeValues MyKeeponControlPanel::syncGazeValues = GazeValues();
MyKeeponControlPanel::DanceValues MyKeeponControlPanel::syncDanceValues = DanceValues();

set<MyKeeponControlPanel*> MyKeeponControlPanel::theSyncGazePanels = set<MyKeeponControlPanel*>();
set<MyKeeponControlPanel*> MyKeeponControlPanel::theSyncDancePanels = set<MyKeeponControlPanel*>();

MyKeeponControlPanel::MyKeeponControlPanel(const ofVec2f p):
mGui(p.x,p.y,0,0) {
	bDelete = false;
	bSerialInited = false;
	bUpdateSerialList = true;
	bIsGazeSync = false;
	bIsDanceSync = false;
	bUpdateGazeGuiFromValues = false;
	bUpdateDanceGuiFromValues = false;
	lastHalfBeat = ofGetElapsedTimeMillis();
	// scripting
	isScriptPlaying = isScriptLoaded = false;
	lastScriptCommand = ofGetElapsedTimeMillis();
	// temporary common dimensions variable
	float tDim = 0;

	//////////////// my GUI
	mGui.setFont("verdana.ttf");
	////// Serial Port list
	mSerialList = (ofxUIDropDownList*) mGui.addWidgetDown(new ofxUIDropDownList("Serial List", updateSerialList()));
	mSerialList->setAutoClose(true);
	tDim = mSerialList->getRect()->height;
	mGui.addSpacer(10*tDim,5);

	////// 2D Pad for Pan/Tilt
	m2DPad = (ofxUI2DPad*) mGui.addWidgetDown(new ofxUI2DPad("Pan/Tilt", ofPoint(0,1), ofPoint(0,1), ofPoint(0.5,0.5),10*tDim,5*tDim));
	////// Motor speeds
	mPanSpeed = (ofxUISlider*) mGui.addWidgetDown(new ofxUISlider("Pan Speed", 0,1, 0.5,10*tDim,tDim));
	mTiltSpeed = (ofxUISlider*) mGui.addWidgetDown(new ofxUISlider("Tilt Speed", 0,1, 0.5,10*tDim,tDim));
	mSideSpeed = (ofxUISlider*) mGui.addWidgetDown(new ofxUISlider("PonSide Speed", 0,1, 0.5,10*tDim,tDim));
	// synch button
	mGui.addWidgetDown(new ofxUIToggle("Synchronize Gaze",false,tDim,tDim,0,0,OFX_UI_FONT_SMALL));
	mGui.addSpacer(10*tDim,5);

	////// Dance interface
	mGui.addWidgetDown(new ofxUILabel("enable double reverse", OFX_UI_FONT_MEDIUM));
	mPanDance = (ofxUIToggleMatrix*) mGui.addWidgetDown(new ofxUIToggleMatrix(3*tDim, tDim, 1, 3, "Pan Dance"));
	mTiltDance = (ofxUIToggleMatrix*) mGui.addWidgetDown(new ofxUIToggleMatrix(3*tDim, tDim, 1, 3, "Tilt Dance"));
	mSideDance = (ofxUIToggleMatrix*) mGui.addWidgetDown(new ofxUIToggleMatrix(3*tDim, tDim, 1, 3, "PonSide Dance"));
	mTempoDance = (ofxUISlider*) mGui.addWidgetDown(new ofxUISlider("Tempo", 0,1, 0.5,10*tDim,tDim));
	// synch button
	mGui.addWidgetDown(new ofxUIToggle("Synchronize Dance",false,tDim,tDim,0,0,OFX_UI_FONT_SMALL));
	mGui.addSpacer(10*tDim,5);

	// script buttons
	mGui.addWidgetDown(new ofxUILabel("Script", OFX_UI_FONT_MEDIUM));
	mLoad = (ofxUIButton*) mGui.addWidgetDown(new ofxUILabelButton("Load", false));
	mPlay = (ofxUIButton*) mGui.addWidgetRight(new ofxUILabelButton("Play",false));
	mGui.addWidgetRight(new ofxUILabelButton("Reset",false));
	mGui.addSpacer(10*tDim,5);

	// remove button
	mGui.addWidgetDown(new ofxUILabelButton("Remove", false));

	// finish gui stuff
	mGui.autoSizeToFitWidgets();
	mGui.setColorBack(ofColor(100,200));
	ofAddListener(mGui.newGUIEvent,this,&MyKeeponControlPanel::guiListener);
}

MyKeeponControlPanel::~MyKeeponControlPanel(){
	mSerial.close();
	theSyncGazePanels.erase(this);
	theSyncDancePanels.erase(this);
}

void MyKeeponControlPanel::update(){
	// check if we have to update the list of serial connections on this panel
	if(bUpdateSerialList){
		mSerialList->clearToggles();
		for(int i=0; i<theSerials.size(); i++){
			mSerialList->addToggle(theSerials.at(i));
		}
		bUpdateSerialList = false;
	}
	// clear serial pipe
	if(bSerialInited) {
		mSerial.flush();
	}
	
	// check if we have to update due to sync
	if(bUpdateGazeGuiFromValues){
		m2DPad->setValue(ofPoint(mGazeValues.pan,mGazeValues.tilt));
		mPanSpeed->setValue(mGazeValues.panSpeed);
		mTiltSpeed->setValue(mGazeValues.tiltSpeed);
		mSideSpeed->setValue(mGazeValues.sideSpeed);
		bUpdateGazeGuiFromValues = false;
	}
	if(bUpdateDanceGuiFromValues) {
		mTempoDance->setValue(mDanceValues.tempo);

		mPanDance->setToggle(0, 0, mDanceValues.pan.enabled);
		mPanDance->setToggle(0, 1, mDanceValues.pan.doubled);
		mPanDance->setToggle(0, 2, mDanceValues.pan.reversed);
		//
		mTiltDance->setToggle(0, 0, mDanceValues.tilt.enabled);
		mTiltDance->setToggle(0, 1, mDanceValues.tilt.doubled);
		mTiltDance->setToggle(0, 2, mDanceValues.tilt.reversed);
		//
		mSideDance->setToggle(0, 0, mDanceValues.side.enabled);
		mSideDance->setToggle(0, 1, mDanceValues.side.doubled);
		mSideDance->setToggle(0, 2, mDanceValues.side.reversed);

		bUpdateDanceGuiFromValues = false;
	}

	unsigned long long tBeat = ofMap(mDanceValues.tempo,0,1, 1000, 500);
	if(ofGetElapsedTimeMillis()-lastHalfBeat > tBeat/2) {
		// whether this is the beat or the half-beat
		bool bOnTheBeat = (ofGetElapsedTimeMillis()%tBeat) < (tBeat/2);

		// pan dance: enabled && (on the beat or double-time)
		if((mDanceValues.pan.enabled) && (bOnTheBeat || mDanceValues.pan.doubled)) {
			// magick to figure out position
			if((bOnTheBeat&&(mDanceValues.pan.doubled||mDanceValues.beatPos)) ^ mDanceValues.pan.reversed) {
				mGazeValues.pan = mDanceValues.panCenter - 0.11;
			}
			else {
				mGazeValues.pan = mDanceValues.panCenter + 0.11;
			}
		}

		// tilt dance: enabled && (on the beat or double-time)
		if((mDanceValues.tilt.enabled) && (bOnTheBeat || mDanceValues.tilt.doubled)) {
			// magick to figure out position
			if((bOnTheBeat&&(mDanceValues.tilt.doubled||mDanceValues.beatPos)) ^ mDanceValues.tilt.reversed) {
				mGazeValues.tilt = mDanceValues.tiltCenter - 0.4;
			}
			else {
				mGazeValues.tilt = mDanceValues.tiltCenter + 0.4;
			}
		}

		// side dance: enabled && (on the beat or double-time)
		if((mDanceValues.side.enabled) && (bOnTheBeat || mDanceValues.side.doubled)) {
			// magick to figure out position
			// mDanceValues.side.reversed selects between pon and side
			//     this is pon
			if(mDanceValues.side.reversed) {
				mGazeValues.side = 2;
			}
			//     this is side
			else {
				if(bOnTheBeat&&(mDanceValues.side.doubled||mDanceValues.beatPos)) {
					mGazeValues.side = 1;
				}
				else {
					mGazeValues.side = -1;
				}
			}
		}

		// send values
		if(mDanceValues.pan.enabled || mDanceValues.tilt.enabled) {
			sendPanAndTilt();
		}
		if(mDanceValues.side.enabled) {
			sendSide();
		}

		// flip beatPos on whole beats
		mDanceValues.beatPos ^= bOnTheBeat;
		lastHalfBeat = ofGetElapsedTimeMillis();
	}
	
	// scripting control
	if(isScriptLoaded && isScriptPlaying) {
		// there's a command to process and its time to process it
		if(mScript.hasCommand() && (ofGetElapsedTimeMillis()>lastScriptCommand+mScript.getDelay())) {
			// send command to serial
			if(bSerialInited) {
				mSerial.writeBytes((unsigned char*)mScript.getCommand().c_str(), mScript.getCommand().size());
			}
			cout << mScript.getDelay() << " " << mScript.getCommand() << endl;
			mScript.getNextCommand();
			lastScriptCommand = ofGetElapsedTimeMillis();
		}
		// no more commands
		else if (!mScript.hasCommand()) {
			mPlay->setColorBack(OFX_UI_COLOR_BACK);
			isScriptPlaying = false;
		}
	}
}

void MyKeeponControlPanel::guiListener(ofxUIEventArgs &args){
	string name = args.widget->getName();
	// Serial list bureaucracy
	if(name.compare("Serial List") == 0) {
		ofxUIDropDownList *ddlist = (ofxUIDropDownList *) args.widget;
		if(ddlist->getSelected().size()) {
			string selection = ddlist->getSelected()[0]->getName();
			if(selection.compare("Refresh List") == 0){
				// refresh static list, but only update dropdown on next call to update()
				updateSerialList();
				bUpdateSerialList = true;
				ddlist->clearSelected();
			}
			// else, if clicked on an item, try to setup a serial connection
			// TODO: add some feedback as to whether we're connected and to what
			else if(!ddlist->isOpen()) {
				mSerial.close();
				bSerialInited = mSerial.setup(selection, 115200);
			}
		}
	}
	// immediate-mode stuff
	else if(name.compare("Pan/Tilt") == 0) {
		mGazeValues.pan = ((ofxUI2DPad *)args.widget)->getScaledValue().x;
		mGazeValues.tilt = ((ofxUI2DPad *)args.widget)->getScaledValue().y;
		// keep dance values up-to-date
		mDanceValues.panCenter = mGazeValues.pan;
		mDanceValues.tiltCenter = mGazeValues.tilt;

		if(!bIsGazeSync) {
			sendPanAndTilt();
		}
		else {
			syncGazeValues = mGazeValues;
			sendSyncPanAndTilt();
		}
	}
	else if(name.compare("Pan Speed") == 0) {
		mGazeValues.panSpeed = ((ofxUISlider *)args.widget)->getScaledValue();
		if(!bIsGazeSync) {
			sendPanSpeed();
		}
		else {
			syncGazeValues = mGazeValues;
			sendSyncPanSpeed();
		}
	}
	else if(name.compare("Tilt Speed") == 0) {
		mGazeValues.tiltSpeed = ((ofxUISlider *)args.widget)->getScaledValue();
		if(!bIsGazeSync) {
			sendTiltSpeed();
		}
		else {
			syncGazeValues = mGazeValues;
			sendSyncTiltSpeed();
		}
	}
	else if(name.compare("PonSide Speed") == 0) {
		mGazeValues.sideSpeed = ((ofxUISlider *)args.widget)->getScaledValue();
		if(!bIsGazeSync) {
			sendSideSpeed();
		}
		else {
			syncGazeValues = mGazeValues;
			sendSyncSideSpeed();
		}
	}

	else if(name.compare("Synchronize Gaze") == 0){
		bIsGazeSync = ((ofxUIButton*)args.widget)->getValue();
		if(bIsGazeSync) {
			// if first item, copy to syncValue
			if(theSyncGazePanels.size() < 1){
				syncGazeValues = mGazeValues;
			}
			else{
				mGazeValues = syncGazeValues;
				// keep dance values up-to-date
				mDanceValues.panCenter = mGazeValues.pan;
				mDanceValues.tiltCenter = mGazeValues.tilt;
				bUpdateGazeGuiFromValues = true;
			}
			// add to set of sync panels
			theSyncGazePanels.insert(this);
		}
		else {
			// remove from vector of sync panels
			theSyncGazePanels.erase(this);
		}
	}

	//////// Dance
	else if(name.compare("Pan Dance(0,0)") == 0) {
		mDanceValues.pan.enabled = !mDanceValues.pan.enabled;
		if(bIsDanceSync) {
			syncDanceValues = mDanceValues;
			syncDance();
		}
	}
	else if(name.compare("Pan Dance(0,1)") == 0) {
		mDanceValues.pan.doubled = !mDanceValues.pan.doubled;
		if(bIsDanceSync) {
			syncDanceValues = mDanceValues;
			syncDance();
		}
	}
	else if(name.compare("Pan Dance(0,2)") == 0) {
		mDanceValues.pan.reversed = !mDanceValues.pan.reversed;
		if(bIsDanceSync) {
			syncDanceValues = mDanceValues;
			syncDance();
		}
	}

	else if(name.compare("Tilt Dance(0,0)") == 0) {
		mDanceValues.tilt.enabled = !mDanceValues.tilt.enabled;
		if(bIsDanceSync) {
			syncDanceValues = mDanceValues;
			syncDance();
		}
	}
	else if(name.compare("Tilt Dance(0,1)") == 0) {
		mDanceValues.tilt.doubled = !mDanceValues.tilt.doubled;
		if(bIsDanceSync) {
			syncDanceValues = mDanceValues;
			syncDance();
		}
	}
	else if(name.compare("Tilt Dance(0,2)") == 0) {
		mDanceValues.tilt.reversed = !mDanceValues.tilt.reversed;
		if(bIsDanceSync) {
			syncDanceValues = mDanceValues;
			syncDance();
		}
	}

	else if(name.compare("PonSide Dance(0,0)") == 0) {
		mDanceValues.side.enabled = !mDanceValues.side.enabled;
		if(bIsDanceSync) {
			syncDanceValues = mDanceValues;
			syncDance();
		}
	}
	else if(name.compare("PonSide Dance(0,1)") == 0) {
		mDanceValues.side.doubled = !mDanceValues.side.doubled;
		if(bIsDanceSync) {
			syncDanceValues = mDanceValues;
			syncDance();
		}
	}
	else if(name.compare("PonSide Dance(0,2)") == 0) {
		mDanceValues.side.reversed = !mDanceValues.side.reversed;
		if(bIsDanceSync) {
			syncDanceValues = mDanceValues;
			syncDance();
		}
	}
	
	else if(name.compare("Tempo") == 0) {
		mDanceValues.tempo = ((ofxUISlider *)args.widget)->getScaledValue();
		if(bIsDanceSync) {
			syncDanceValues = mDanceValues;
			syncDance();
		}
	}

	else if(name.compare("Synchronize Dance") == 0){
		bIsDanceSync = ((ofxUIButton*)args.widget)->getValue();
		if(bIsDanceSync) {
			// if first item, copy to syncValue
			if(theSyncDancePanels.size() < 1){
				syncDanceValues = mDanceValues;
			}
			else{
				mDanceValues = syncDanceValues;
				bUpdateDanceGuiFromValues = true;
			}
			// add to set of sync panels
			theSyncDancePanels.insert(this);
		}
		else {
			// remove from vector of sync panels
			theSyncDancePanels.erase(this);
		}
	}

	//////// Script
	else if((name.compare("Load") == 0) && (((ofxUIButton*)args.widget)->getValue())) {
		// just in case
		isScriptLoaded = false;
		mLoad->setColorBack(OFX_UI_COLOR_BACK);
		// open XML file
		ofFileDialogResult mFDR = ofSystemLoadDialog("Pick an xml script file", false, ofToDataPath("",true));
		if(mFDR.bSuccess) {
			mScript.loadScript(mFDR.getName());
			isScriptLoaded = true;
			mLoad->setColorBack(ofColor(0,100,0));
			isScriptPlaying = false;
		}
	}
	else if((name.compare("Play") == 0) && (((ofxUIButton*)args.widget)->getValue())) {
		isScriptPlaying = !isScriptPlaying;
		if(isScriptLoaded && isScriptPlaying) {
			lastScriptCommand = ofGetElapsedTimeMillis();
			((ofxUIButton*)args.widget)->setColorBack(ofColor(0,100,0));
		}
		else if(isScriptLoaded && !isScriptPlaying) {
			((ofxUIButton*)args.widget)->setColorBack(ofColor(100,0,0));
		}
	}
	else if((name.compare("Reset") == 0) && (((ofxUIButton*)args.widget)->getValue())) {
		mScript.reset();
		isScriptPlaying = false;
		mPlay->setColorBack(OFX_UI_COLOR_BACK);
	}

	//////// 
	else if((name.compare("Remove") == 0) && (((ofxUIButton*)args.widget)->getValue())){
		bDelete = true;
	}
}

const bool& MyKeeponControlPanel::toDelete() const{
	return bDelete;
}

void MyKeeponControlPanel::setX(const int x_){
	mGui.setPosition(x_, mGui.getRect()->y);
}

const ofRectangle MyKeeponControlPanel::getRectangle() {
	return ofRectangle(mGui.getRect()->x,mGui.getRect()->y,mGui.getRect()->width,mGui.getRect()->height);
}

//// helpers
void MyKeeponControlPanel::sendPanAndTilt() {
	if(bSerialInited){
		string msg = "MOVE PAN "+ofToString((int)ofMap(mGazeValues.pan, 0,1, -90,90, true))+";";
		mSerial.writeBytes((unsigned char*)msg.c_str(), msg.size());
		msg = "MOVE TILT "+ofToString((int)ofMap(mGazeValues.tilt, 0,1, -90,90, true))+";";
		mSerial.writeBytes((unsigned char*)msg.c_str(), msg.size());
	}
}
void MyKeeponControlPanel::sendSide() {
	if(bSerialInited){
		string msg;
		switch((int)mGazeValues.side) {
			case -1: {
				msg = "MOVE SIDE LEFT;";
				break;
			}
			case 1: {
				msg = "MOVE SIDE RIGHT;";
				break;
			}
			case -2: {
				msg = "MOVE PON DOWN;";
				break;
			}
			case 2: {
				msg = "MOVE PON UP;";
				break;
			}
			default: {
				msg = "MOVE PON UP;";
			}
		}
		mSerial.writeBytes((unsigned char*)msg.c_str(), msg.size());
	}
}
void MyKeeponControlPanel::sendPanSpeed() {
	if(bSerialInited) {
		string msg = "SPEED PAN "+ofToString((int)ofMap(mGazeValues.panSpeed, 0,1, 64,250, true))+";";
		mSerial.writeBytes((unsigned char*)msg.c_str(), msg.size());
	}
}
void MyKeeponControlPanel::sendTiltSpeed() {
	if(bSerialInited) {
		string msg = "SPEED TILT "+ofToString((int)ofMap(mGazeValues.tiltSpeed, 0,1, 64,250, true))+";";
		mSerial.writeBytes((unsigned char*)msg.c_str(), msg.size());
	}
}
void MyKeeponControlPanel::sendSideSpeed() {
	if(bSerialInited) {
		string msg = "SPEED PONSIDE "+ofToString((int)ofMap(mGazeValues.sideSpeed, 0,1, 64,250, true))+";";
		mSerial.writeBytes((unsigned char*)msg.c_str(), msg.size());
	}
}

//// static helpers
void MyKeeponControlPanel::sendSyncPanAndTilt() {
	for(set<MyKeeponControlPanel*>::const_iterator it=theSyncGazePanels.begin(); it!=theSyncGazePanels.end(); ++it){
		(*it)->mGazeValues = syncGazeValues;
		(*it)->sendPanAndTilt();
		(*it)->bUpdateGazeGuiFromValues = true;
	}
}
void MyKeeponControlPanel::sendSyncPanSpeed() {
	for(set<MyKeeponControlPanel*>::const_iterator it=theSyncGazePanels.begin(); it!=theSyncGazePanels.end(); ++it){
		(*it)->mGazeValues = syncGazeValues;
		(*it)->sendPanSpeed();
		(*it)->bUpdateGazeGuiFromValues = true;
	}
}
void MyKeeponControlPanel::sendSyncTiltSpeed() {
	for(set<MyKeeponControlPanel*>::const_iterator it=theSyncGazePanels.begin(); it!=theSyncGazePanels.end(); ++it){
		(*it)->mGazeValues = syncGazeValues;
		(*it)->sendTiltSpeed();
		(*it)->bUpdateGazeGuiFromValues = true;
	}
}
void MyKeeponControlPanel::sendSyncSideSpeed() {
	for(set<MyKeeponControlPanel*>::const_iterator it=theSyncGazePanels.begin(); it!=theSyncGazePanels.end(); ++it){
		(*it)->mGazeValues = syncGazeValues;
		(*it)->sendSideSpeed();
		(*it)->bUpdateGazeGuiFromValues = true;
	}
}

void MyKeeponControlPanel::syncDance() {
	for(set<MyKeeponControlPanel*>::const_iterator it=theSyncDancePanels.begin(); it!=theSyncDancePanels.end(); ++it){
		(*it)->mDanceValues = syncDanceValues;
		(*it)->bUpdateDanceGuiFromValues = true;
	}
}
