#include "ScriptReader.h"

ScriptReader::ScriptReader() {
	this->reset();
}
ScriptReader::~ScriptReader() {}

void ScriptReader::loadScript(const string fileName) {
	theXml.loadFile(fileName);
	this->reset();
}

const string& ScriptReader::getCommand() const {
	return currCommand;
}
const unsigned int& ScriptReader::getDelay() const {
	return currDelay;
}

// currCommandIndex can point to one past last tag
void ScriptReader::getNextCommand() {
	if(currCommandIndex < (theXml.getNumTags("cmd")-1)) {
		currCommandIndex++;
		currCommand = theXml.getValue("cmd", "", currCommandIndex);
		currDelay = theXml.getAttribute("cmd", "time", 1e10, currCommandIndex);
	}
	else if(currCommandIndex < theXml.getNumTags("cmd")) {
		currCommandIndex++;
		// currCommandIndex now points beyond last tag
	}
}

const bool ScriptReader::hasCommand() {
	return (currCommandIndex < theXml.getNumTags("cmd"));
}

void ScriptReader::reset() {
	currCommandIndex = 0;
	currCommand = "";
	currDelay = 1e10;

	// see if the doc is loaded...
	if(theXml.getNumTags("cmd") > 0) {
		currCommand = theXml.getValue("cmd", "", currCommandIndex);
		currDelay = theXml.getAttribute("cmd", "time", 1e10, currCommandIndex);
	}
}

