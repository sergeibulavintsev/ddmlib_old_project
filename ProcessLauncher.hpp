#ifndef PROCESSLAUNCHER_HPP
#define PROCESSLAUNCHER_HPP

#include "ddmlib.hpp"


namespace ddmlib {

	class DetachedProcessHandle : public Poco::ProcessHandle {
	public:
		DetachedProcessHandle(Poco::ProcessHandleImpl* pImpl);
		friend class DetachedProcessWin32;
	private:
		DetachedProcessHandle();

		Poco::ProcessHandleImpl* _pImpl;
	};

	class DetachedProcessWin32 : public Poco::Process {
	public:
		typedef ArgsImpl Args;
		static Poco::ProcessHandleImpl* launchImpl(const std::string& command, const ArgsImpl& args, Poco::Pipe* inPipe, Poco::Pipe* outPipe, Poco::Pipe* errPipe);	
		
		static DetachedProcessHandle launch(const std::string& command, const Args& args);
		static DetachedProcessHandle launch(const std::string& command, const Args& args, Poco::Pipe* inPipe, Poco::Pipe* outPipe, Poco::Pipe* errPipe);
	};
}

#endif