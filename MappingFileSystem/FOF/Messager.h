#pragma once
#include "Base.h"
#include "NodeProcesser.h"

class Messager : public EventCallback {
	FileWriter * fw;
public:
	~Messager() {
		if(fw!=NULL)delete fw;
	}
	Messager() {
		fw = new FileWriter();
		std::string timestring = getTimeToString();
		timestring = timestring.substr(0, timestring.find_first_of("ÈÕ")).append(".log").insert(0, ".\\Data\\Log\\");
		std::string outpath = ".\\Data\\Log\\";
		MakeOutDirs(outpath,outpath);
		if (IsOutFileExist(timestring)) {
			fw->Open((char*)timestring.c_str(),34 | std::ios_base::app);//".\\Info.log"
		}
		else {
			fw->Open((char*)timestring.c_str());
		}
		
	}

	virtual void OnEventFinish(int status, std::string Exinfo) override
	{
	
	}
	// Í¨¹ý EventCallback ¼Ì³Ð
	virtual void OnError( int ErrorCode, std::string Msg, std::string Exinfo) override {
		std::stringstream sst;
		sst << getTimeToString()<< " Code:" << ErrorCode<<"¡£" <<" Error: " << Msg << " " << Exinfo;
		fw->WriteLine(&sst.str());
	};
	virtual void OnWarring(int Code, std::string Msg, std::string Exinfo) override {
		std::stringstream sst;
		sst << getTimeToString() << " Code:" << Code<< "¡£"  << " Warring: " << Msg << " " << Exinfo;
		fw->WriteLine(&sst.str());
	}
};