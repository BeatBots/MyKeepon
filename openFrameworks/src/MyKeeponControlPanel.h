#pragma once

#include "ofRectangle.h"
#include "ofxUI.h"
#include "ScriptReader.h"
#include <set>

class MyKeeponControlPanel{
	public:
		MyKeeponControlPanel(const ofVec2f p);
		~MyKeeponControlPanel();
		void update();
		void guiListener(ofxUIEventArgs &args);
		const bool& toDelete() const;
		void setX(const int x_);
		const ofRectangle getRectangle();
		struct GazeValues {
			float pan = 0.5, tilt = 0.5, side=0;
			float panSpeed = 0.5, tiltSpeed = 0.5, sideSpeed = 0.5;
		};
		struct DanceValues {
			struct DanceArray {
				bool enabled=false, doubled=false, reversed=false;
			};
			DanceArray pan, tilt, side;
			bool beatPos = false;
			float tempo = 0.5;
			float panCenter = 0.5, tiltCenter = 0.5;
		};
	private:
		// GUI handlers
		ofxUICanvas mGui;
		ofxUIDropDownList* mSerialList;
		ofxUI2DPad* m2DPad;
		ofxUISlider *mPanSpeed, *mTiltSpeed, *mSideSpeed, *mTempoDance;
		ofxUIToggleMatrix *mPanDance, *mTiltDance, *mSideDance;
		ofxUIButton *mPlay, *mLoad;
		// Serial
		ofSerial mSerial;
		// bools
		bool bDelete, bUpdateSerialList, bSerialInited, bIsGazeSync, bIsDanceSync, bUpdateGazeGuiFromValues, bUpdateDanceGuiFromValues;
		GazeValues mGazeValues;
		// helpers
		void sendPanAndTilt();
		void sendSide();
		void sendPanSpeed();
		void sendTiltSpeed();
		void sendSideSpeed();
		// dance state variables
		DanceValues mDanceValues;
		unsigned long long lastHalfBeat;
		// scripted dance variables
		ScriptReader mScript;
		bool isScriptPlaying, isScriptLoaded;
		unsigned long long lastScriptCommand;

		// statics
		static vector<string> theSerials;
		static vector<string>& updateSerialList();
		static GazeValues syncGazeValues;
		static DanceValues syncDanceValues;
		static set<MyKeeponControlPanel*> theSyncGazePanels, theSyncDancePanels;
		static void sendSyncPanAndTilt();
		static void sendSyncPanSpeed();
		static void sendSyncTiltSpeed();
		static void sendSyncSideSpeed();
		static void syncDance();
};
