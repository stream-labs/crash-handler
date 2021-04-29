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

#include <windows.h>
#include <CommCtrl.h>
#include <thread>

#include "../logger.hpp"
#include "upload-window-win.hpp"

UploadWindow* UploadWindow::instance = NULL;

#define CUSTOM_CLOSE_MSG (WM_USER + 1)
#define CUSTOM_PROGRESS_MSG (WM_USER + 2)

#define CUSTOM_CAUGHT_CRASH (WM_USER + 3)
#define CUSTOM_SAVE_STARTED (WM_USER + 4)
#define CUSTOM_SAVED_DUMP (WM_USER + 5)
#define CUSTOM_SAVING_DUMP_FAILED (WM_USER + 6)
#define CUSTOM_UPLOAD_STARTED (WM_USER + 7)
#define CUSTOM_UPLOAD_FINISHED (WM_USER + 8)
#define CUSTOM_UPLOAD_FAILED (WM_USER + 9)
#define CUSTOM_UPLOAD_CANCELED (WM_USER + 10)

#define CUSTOM_REQUESTED_CLICK (WM_USER + 12)

LRESULT CALLBACK FrameWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_CLOSE:
		return 0;
	case WM_NCCREATE:
	case WM_CREATE: {
		break;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_NCDESTROY:
		break;
	default: {
		return UploadWindow::getInstance()->WndProc(hwnd, msg, wParam, lParam);
	}
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void UploadWindow::showButtons(bool ok, bool yes, bool cancel, bool no)
{
	ShowWindow(ok_button_hwnd, ok ? SW_SHOW:SW_HIDE);
	ShowWindow(yes_button_hwnd, yes ? SW_SHOW:SW_HIDE);
	ShowWindow(cancel_button_hwnd, cancel ? SW_SHOW:SW_HIDE);
	ShowWindow(no_button_hwnd, no ? SW_SHOW:SW_HIDE);
}

void UploadWindow::enableButtons(bool ok, bool yes, bool cancel, bool no)
{
	EnableWindow(ok_button_hwnd, ok );
	EnableWindow(yes_button_hwnd, yes );
	EnableWindow(cancel_button_hwnd, cancel );
	EnableWindow(no_button_hwnd, no );
}

LRESULT UploadWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (upload_window_hwnd == NULL) {
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	switch (msg) {
	case CUSTOM_CLOSE_MSG:
		log_info << "UploadWindow close message recieved" << std::endl;
		DestroyWindow(upload_window_hwnd);
		upload_window_hwnd = NULL;
		break;
	case CUSTOM_CAUGHT_CRASH: {
		SetWindowText(upload_label_hwnd, L"The application just crashed.\r\n\r\n"
		"Would you like to share additional information to the developers?");
		showButtons(false, true, true, false);
		enableButtons(false, true, true, false);
		SendMessage(progresss_bar_hwnd, PBM_SETBARCOLOR, 0, RGB(49, 195, 162));
		SendMessage(progresss_bar_hwnd, PBM_SETRANGE32, 0, 100);
		SendMessage(progresss_bar_hwnd, PBM_SETPOS, 0, 0);
		ShowWindow(progresss_bar_hwnd, SW_SHOW);
		break;
	}
	case CUSTOM_PROGRESS_MSG: {
		double progress = ((double)bytes_sent) / ((double)total_bytes_to_send / 100.0);
		PostMessage(progresss_bar_hwnd, PBM_SETPOS, (int)progress, 0);
		swprintf(upload_progress_message, upload_message_len, L"%lld / %lld Kb", bytes_sent / 1024, total_bytes_to_send / 1024);
		SetWindowText(upload_label_hwnd, upload_progress_message);
		break;
	}
	case CUSTOM_SAVED_DUMP: {
		swprintf(upload_progress_message, upload_message_len, L"Additional debug information saved successfully.\r\n\r\n"
		"Continue with upload?\r\n\r\n"
		"File size: %.1fMb", (float)total_bytes_to_send/1024/1024);
		SetWindowText(upload_label_hwnd, upload_progress_message);
		showButtons(false, true, true, false);
		enableButtons(false, true, true, false);
		break;
	}
	case CUSTOM_SAVE_STARTED: {
		swprintf(upload_progress_message, upload_message_len, L"Saving... to \"%s\"", file_name.c_str());
		SetWindowText(upload_label_hwnd, upload_progress_message);
		showButtons(false, false, false, false);
		break;
	}
	case CUSTOM_SAVING_DUMP_FAILED: {
		SetWindowText(upload_label_hwnd, L"Failed to save the local debug information.");
		showButtons(true, false, true, false);
		enableButtons(true, false, false, false);
		break;
	}
	case CUSTOM_UPLOAD_STARTED: {
		SendMessage(progresss_bar_hwnd, PBM_SETRANGE32, 0, 100);
		SetWindowText(upload_label_hwnd, L"Initializing upload...");
		showButtons(false, false, false, false);
		break;
	}
	case CUSTOM_UPLOAD_FINISHED: {
		swprintf(upload_progress_message, upload_message_len, L"Successfully uploaded the additional debug information.\r\n\r\n"
		"Please provide this file name to the support.\r\n\r\n"
		"File: \"%s\".", file_name.c_str());
		SetWindowText(upload_label_hwnd, upload_progress_message);
		showButtons(true, false, true, false);
		enableButtons(true, false, false, false);
		break;
	}
	case CUSTOM_UPLOAD_CANCELED: {
		swprintf(upload_progress_message, upload_message_len, L"Upload cancleled.\r\n"
		"Would you like to remove the file from your system( Press No) ?\r\n"
		"Or keep it to share with our support team (Press Yes)?\r\n"
		"Located: \"%%APPDATA%%\\CrashMemoryDump\\%s\".", file_name.c_str());
		SetWindowText(upload_label_hwnd, upload_progress_message);
		showButtons(false, true, false, true);
		enableButtons(false, true, false, true);
		break;
	}
	case CUSTOM_UPLOAD_FAILED: {
		swprintf(upload_progress_message, upload_message_len, L"Upload failed.\r\n"
		"Would you like to remove the file from your system( Press No) ?\r\n"
		"Or keep it to share with our support team (Press Yes)?\r\n"
		"Located: \"%%APPDATA%%\\CrashMemoryDump\\%s\".", file_name.c_str());
		SetWindowText(upload_label_hwnd, upload_progress_message);
		showButtons(false, true, false, true);
		enableButtons(false, true, false, true);
		break;
	}
	case WM_COMMAND: {
		if ((HWND)lParam == ok_button_hwnd) {
			showButtons(false, false, false, false);
			SetWindowText(upload_label_hwnd, L"");
			button_clicked = IDOK;
			upload_window_choose_variable.notify_one();
			break;
		}
		if ((HWND)lParam == yes_button_hwnd) {
			showButtons(false, false, false, false);
			SetWindowText(upload_label_hwnd, L"");
			button_clicked = IDYES;
			upload_window_choose_variable.notify_one();
			break;
		}
		if ((HWND)lParam == cancel_button_hwnd) {
			showButtons(false, false, false, false);
			SetWindowText(upload_label_hwnd, L"");
			button_clicked = IDCANCEL;
			upload_window_choose_variable.notify_one();
			break;
		}
		if ((HWND)lParam == no_button_hwnd) {
			showButtons(false, false, false, false);
			SetWindowText(upload_label_hwnd, L"");
			button_clicked = IDNO;
			upload_window_choose_variable.notify_one();
			break;
		}
		break;
	}
	case CUSTOM_REQUESTED_CLICK: {
		if (!upload_window_hwnd) {
			button_clicked = IDOK;
			upload_window_choose_variable.notify_one();
		} else {
			if (button_clicked != 0) {
				upload_window_choose_variable.notify_one();
			}
		}
		break;
	}
	case WM_CTLCOLORSTATIC:
	case WM_CTLCOLOREDIT:
	{
		if ((HWND)lParam == upload_label_hwnd) {
			HDC hdc = (HDC)wParam;
			return (LRESULT)GetStockObject(CTLCOLOR_MSGBOX);
		}
	}
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

UploadWindow::UploadWindow() {}

UploadWindow::~UploadWindow()
{
	if (window_thread) {
		PostMessage(upload_window_hwnd, CUSTOM_CLOSE_MSG, 0, 0);
		if (window_thread->joinable()) {
			window_thread->join();
		}
		window_thread = nullptr;
	}
};

UploadWindow* UploadWindow::getInstance()
{
	if (instance)
		return instance;
	instance = new UploadWindow();
	return instance;
}

void UploadWindow::shutdownInstance()
{
	delete instance;
	instance = nullptr;
}

void UploadWindow::windowThread()
{
	log_info << "UploadWindow windowThread started" << std::endl;
	hInstance = GetModuleHandle(NULL);

	WNDCLASSEX wc;
	wc.cbSize        = sizeof(WNDCLASSEX);
	wc.style         = CS_NOCLOSE;
	wc.lpfnWndProc   = FrameWndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(CTLCOLOR_MSGBOX);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = L"uploaderwindowclass";

	if (!RegisterClassEx(&wc)) {
		log_error << "Failed to create a class for uploader window " << GetLastError() << std::endl;
		upload_window_choose_variable.notify_one();
		return;
	}

	/* We only care about the main display */
	int screen_width  = GetSystemMetrics(SM_CXSCREEN);
	int screen_height = GetSystemMetrics(SM_CYSCREEN);

	upload_window_hwnd = CreateWindowEx(
	    WS_EX_CLIENTEDGE,
	    L"uploaderwindowclass",
	    L"Send additional debug information to the developers.",
	    WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU,
	    (screen_width - width) / 2,
	    (screen_height - height) / 2,
	    width,
	    height,
	    NULL,
	    NULL,
	    hInstance,
	    NULL);
	if (!upload_window_hwnd) {
		log_error << "Failed to create an uploader window " << GetLastError() << std::endl;
		upload_window_choose_variable.notify_one();
		return;
	}

	int  x_pos  = 10;
	int  y_pos  = 10;
	int  x_size = 470;
	int  y_size = 250;
	RECT client_rect;
	if (GetClientRect(upload_window_hwnd, &client_rect)) {
		x_size = client_rect.right - client_rect.left;
		y_size = client_rect.bottom - client_rect.top;
	}

	progresss_bar_hwnd = CreateWindow(
	    PROGRESS_CLASS,
	    TEXT("ProgressWorker"),
	    WS_CHILD | PBS_SMOOTH,
	    x_pos,
	    y_pos,
	    x_size - 20,
	    40,
	    upload_window_hwnd,
	    NULL,
	    NULL,
	    NULL);

	upload_label_hwnd = CreateWindow(
	    WC_EDIT,
	    TEXT(""),
	    WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN,
	    x_pos,
	    y_pos + 50,
	    x_size - 20,
	    90,
	    upload_window_hwnd,
	    NULL,
	    NULL,
	    NULL);

	ok_button_hwnd = CreateWindow(
	    WC_BUTTON,
	    L"OK",
	    WS_TABSTOP | WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
	    x_size - 220,
	    y_size - 50,
	    100,
	    40,
	    upload_window_hwnd,
	    NULL,
	    NULL,
	    NULL);

	yes_button_hwnd = CreateWindow(
	    WC_BUTTON,
	    L"Yes",
	    WS_TABSTOP | WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
	    x_size - 220,
	    y_size - 50,
	    100,
	    40,
	    upload_window_hwnd,
	    NULL,
	    NULL,
	    NULL);

	cancel_button_hwnd = CreateWindow(
	    WC_BUTTON,
	    L"Cancel",
	    WS_TABSTOP | WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
	    x_size - 110,
	    y_size - 50,
	    100,
	    40,
	    upload_window_hwnd,
	    NULL,
	    NULL,
	    NULL);

	no_button_hwnd = CreateWindow(
	    WC_BUTTON,
	    L"No",
	    WS_TABSTOP | WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
	    x_size - 110,
	    y_size - 50,
	    100,
	    40,
	    upload_window_hwnd,
	    NULL,
	    NULL,
	    NULL);

	ShowWindow(upload_window_hwnd, SW_SHOWNORMAL);
	UpdateWindow(upload_window_hwnd);
	showButtons(false, false, false, false);

	HFONT hFont = CreateFont(0, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
	SendMessage (upload_label_hwnd, WM_SETFONT, WPARAM (hFont), TRUE);
	SendMessage (ok_button_hwnd, WM_SETFONT, WPARAM (hFont), TRUE);
	SendMessage (yes_button_hwnd, WM_SETFONT, WPARAM (hFont), TRUE);
	SendMessage (cancel_button_hwnd, WM_SETFONT, WPARAM (hFont), TRUE);
	SendMessage (no_button_hwnd, WM_SETFONT, WPARAM (hFont), TRUE);

	SendMessage (upload_label_hwnd, EM_SETREADONLY, TRUE, TRUE);

	upload_window_choose_variable.notify_one();
	MSG msg;

	while (GetMessage(&msg, NULL, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	DeleteObject(hFont);

	log_info << "UploadWindow windowThread at finish" << std::endl;
}

bool UploadWindow::createWindow()
{
	window_thread = new std::thread(&UploadWindow::windowThread, this);

	std::unique_lock<std::mutex> lock(upload_window_choose_mutex);
	upload_window_choose_variable.wait(lock);

	if (!window_thread || !window_thread->joinable()) {
		return false;
	}

	if (window_thread && window_thread->joinable() && !upload_window_hwnd) {
		return false;
	}

	return true;
}

void UploadWindow::uploadFinished()
{
	button_clicked = 0;
	PostMessage(upload_window_hwnd, CUSTOM_UPLOAD_FINISHED, NULL, NULL);
}

void UploadWindow::uploadStarted()
{
	button_clicked = 0;
	PostMessage(upload_window_hwnd, CUSTOM_UPLOAD_STARTED, NULL, NULL);
}

void UploadWindow::uploadFailed()
{
	button_clicked = 0;
	PostMessage(upload_window_hwnd, CUSTOM_UPLOAD_FAILED, NULL, NULL);
}

void UploadWindow::uploadCanceled()
{
	button_clicked = 0;
	PostMessage(upload_window_hwnd, CUSTOM_UPLOAD_CANCELED, NULL, NULL);
}

void UploadWindow::savingFinished()
{
	button_clicked = 0;
	PostMessage(upload_window_hwnd, CUSTOM_SAVED_DUMP, NULL, NULL);
}

void UploadWindow::savingStarted()
{
	button_clicked = 0;
	PostMessage(upload_window_hwnd, CUSTOM_SAVE_STARTED, NULL, NULL);
}

void UploadWindow::savingFailed()
{
	button_clicked = 0;
	PostMessage(upload_window_hwnd, CUSTOM_SAVING_DUMP_FAILED, NULL, NULL);
}

void UploadWindow::crashCaught()
{
	button_clicked = 0;
	PostMessage(upload_window_hwnd, CUSTOM_CAUGHT_CRASH, NULL, NULL);
}

int UploadWindow::waitForUserChoise()
{
	if (button_clicked != 0) {
		return button_clicked;
	}

	std::unique_lock<std::mutex> lock(upload_window_choose_mutex);
	PostMessage(upload_window_hwnd, CUSTOM_REQUESTED_CLICK, NULL, NULL);
	upload_window_choose_variable.wait(lock);

	return button_clicked;
}

void UploadWindow::setDumpFileName(const std::wstring& new_file_name)
{
	file_name = new_file_name;
}

void UploadWindow::setTotalBytes(long long new_total)
{
	total_bytes_to_send = new_total;
}

void UploadWindow::setUploadProgress(long long sent_amount)
{
	bytes_sent = sent_amount;
	PostMessage(upload_window_hwnd, CUSTOM_PROGRESS_MSG, NULL, NULL);
}
