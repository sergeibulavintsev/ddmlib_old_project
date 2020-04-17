#include "ddmlib.hpp"
#include "ProcessLauncher.hpp"

namespace ddmlib {
	using namespace ::Poco;


	DetachedProcessHandle::DetachedProcessHandle(ProcessHandleImpl* pImpl):
		ProcessHandle(pImpl)
	{
	}


	DetachedProcessHandle DetachedProcessWin32::launch(const std::string& command, const Args& args)
	{
		return DetachedProcessHandle(launchImpl(command, args, 0, 0, 0));
	}


	DetachedProcessHandle DetachedProcessWin32::launch(const std::string& command, const Args& args, Pipe* inPipe, Pipe* outPipe, Pipe* errPipe)
	{
		poco_assert (inPipe == 0 || (inPipe != outPipe && inPipe != errPipe));

		return DetachedProcessHandle(launchImpl(command, args, inPipe, outPipe, errPipe));
	}

	// from Poco sources, added DETACHED_PROCESS flag to CreateProcessW call
	ProcessHandleImpl* DetachedProcessWin32::launchImpl(const std::string& command, const ArgsImpl& args, Pipe* inPipe, Pipe* outPipe, Pipe* errPipe) {
		std::string commandLine = command;
		for (ArgsImpl::const_iterator it = args.begin(); it != args.end(); ++it)
		{
			commandLine.append(" ");
			commandLine.append(*it);
		}		

		std::wstring ucommandLine;
		UnicodeConverter::toUTF16(commandLine, ucommandLine);

		STARTUPINFOW startupInfo;
		GetStartupInfoW(&startupInfo); // take defaults from current process
		startupInfo.cb          = sizeof(STARTUPINFO);
		startupInfo.lpReserved  = NULL;
		startupInfo.lpDesktop   = NULL;
		startupInfo.lpTitle     = NULL;
		startupInfo.dwFlags     = STARTF_FORCEOFFFEEDBACK | STARTF_USESTDHANDLES;
		startupInfo.cbReserved2 = 0;
		startupInfo.lpReserved2 = NULL;
		
		HANDLE hProc = GetCurrentProcess();
		if (inPipe)
		{
			DuplicateHandle(hProc, inPipe->readHandle(), hProc, &startupInfo.hStdInput, 0, TRUE, DUPLICATE_SAME_ACCESS);
			inPipe->close(Pipe::CLOSE_READ);
		}
		else DuplicateHandle(hProc, GetStdHandle(STD_INPUT_HANDLE), hProc, &startupInfo.hStdInput, 0, TRUE, DUPLICATE_SAME_ACCESS);
		// outPipe may be the same as errPipe, so we duplicate first and close later.
		if (outPipe)
			DuplicateHandle(hProc, outPipe->writeHandle(), hProc, &startupInfo.hStdOutput, 0, TRUE, DUPLICATE_SAME_ACCESS);
		else
			DuplicateHandle(hProc, GetStdHandle(STD_OUTPUT_HANDLE), hProc, &startupInfo.hStdOutput, 0, TRUE, DUPLICATE_SAME_ACCESS);
		if (errPipe)
			DuplicateHandle(hProc, errPipe->writeHandle(), hProc, &startupInfo.hStdError, 0, TRUE, DUPLICATE_SAME_ACCESS);
		else
			DuplicateHandle(hProc, GetStdHandle(STD_ERROR_HANDLE), hProc, &startupInfo.hStdError, 0, TRUE, DUPLICATE_SAME_ACCESS);
		if (outPipe) outPipe->close(Pipe::CLOSE_WRITE);
		if (errPipe) errPipe->close(Pipe::CLOSE_WRITE);

		PROCESS_INFORMATION processInfo;
		BOOL rc = CreateProcessW(
			NULL, 
			const_cast<wchar_t*>(ucommandLine.c_str()), 
			NULL, 
			NULL, 
			TRUE, 
			DETACHED_PROCESS, 
			NULL, 
			NULL, 
			&startupInfo, 
			&processInfo
		);
		CloseHandle(startupInfo.hStdInput);
		CloseHandle(startupInfo.hStdOutput);
		CloseHandle(startupInfo.hStdError);
		if (rc)
		{
			CloseHandle(processInfo.hThread);
			return new ProcessHandleImpl(processInfo.hProcess, processInfo.dwProcessId);
		}
		else throw SystemException("Cannot launch process", command);
	}


}