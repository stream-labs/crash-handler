/******************************************************************************
	Copyright (C) 2016-2020 by Streamlabs (General Workings Inc)

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

******************************************************************************/

#ifndef UPLOADWINDOWUTIL_H
#define UPLOADWINDOWUTIL_H
#include <mutex>

class UploadWindow
{
	public:
	static UploadWindow* getInstance();
	static void          shutdownInstance();

	bool createWindow();
	int  waitForUserChoise();
	void setDumpFileName(const std::wstring& new_file_name);
	void setTotalBytes(long long);
	void setUploadProgress(long long);

	void crashCatched();
	void savingFinished();
	void savingFailed();
	void savingStarted();
	void uploadFinished();
	void uploadFailed();
	void uploadCanceled();
	void uploadStarted();

	UploadWindow();
	virtual ~UploadWindow();

	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	
	private:
	void enableButtons(bool ok_enabled, bool yes_enabled, bool cancel_enabled);
	void windowThread();
	static UploadWindow* instance;

	HINSTANCE hInstance          = NULL;
	HWND      upload_window_hwnd = NULL;
	HWND      progresss_bar_hwnd = NULL;
	HWND      upload_label_hwnd  = NULL;
	HWND      ok_button_hwnd     = NULL;
	HWND      yes_button_hwnd    = NULL;
	HWND      cancel_button_hwnd = NULL;
	int       width              = 500;
	int       height             = 250;

	int                     button_clicked      = 0;
	long long               total_bytes_to_send = 0;
	long long               bytes_sent          = 0;
	std::wstring            file_name;
	std::thread*            window_thread = nullptr;
	std::mutex              upload_window_choose_mutex;
	std::condition_variable upload_window_choose_variable;
	TCHAR                   upload_progress_message[128];
};

#endif