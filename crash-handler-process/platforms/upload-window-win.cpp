
#include <windows.h>
#include <CommCtrl.h>

#include <thread>

#include "upload-window-win.hpp"
#include "../logger.hpp"

UploadWindow * UploadWindow::instance = NULL;

#define CUSTOM_CLOSE_MSG (WM_USER + 1)
#define CUSTOM_PROGRESS_MSG (WM_USER + 2)

#define CUSTOM_CATCHED_CRASH (WM_USER + 3)
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
	switch (msg)
	{
                case WM_CLOSE:
        		return 0;
                case WM_NCCREATE:
                case WM_CREATE: {
			break;
                }
                default: {
			return UploadWindow::getInstance()->WndProc(hwnd, msg,  wParam, lParam);
                }

        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT UploadWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
        switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case CUSTOM_CLOSE_MSG:
		DestroyWindow(hwnd);
		break;
	case CUSTOM_CATCHED_CRASH:
	{
		SetWindowText(upload_label_hwnd, L"Crash catched. Start saving a memory dump to a file?");
		EnableWindow(ok_button_hwnd, true);
		EnableWindow(cancel_button_hwnd, true);
		SendMessage(progresss_bar_hwnd, PBM_SETBARCOLOR, 0, RGB(49, 195, 162));
                SendMessage(progresss_bar_hwnd, PBM_SETBKCOLOR,  0, RGB(49, 49, 49));
                SendMessage(progresss_bar_hwnd, PBM_SETRANGE32, 0, 100);
                SendMessage(progresss_bar_hwnd, PBM_SETPOS, 0, 0);
                ShowWindow(progresss_bar_hwnd, SW_SHOW);
		break;
	}		
	case CUSTOM_PROGRESS_MSG:
	{
                double progress = ((double)bytes_sent) / ((double) total_bytes_to_send / 100.0);
		PostMessage(progresss_bar_hwnd, PBM_SETPOS, (int)progress, 0);
		swprintf(upload_progress_message, L"%lld / %lld Kb", bytes_sent/1024, total_bytes_to_send/1024);
		SetWindowText(upload_label_hwnd, upload_progress_message);
		break;
	}
	case CUSTOM_SAVED_DUMP:
	{
		SetWindowText(upload_label_hwnd, L"Memory dump saved successfully. Continue with upload?");
		EnableWindow(ok_button_hwnd, true);
		EnableWindow(cancel_button_hwnd, true);
		break;
	}	
        case CUSTOM_SAVE_STARTED:
	{
                swprintf(upload_progress_message,  L"Saving... to \"%s\"", file_name.c_str());
		SetWindowText(upload_label_hwnd, upload_progress_message);
		EnableWindow(ok_button_hwnd, false);
		EnableWindow(cancel_button_hwnd, false);
		break;
	}
	case CUSTOM_SAVING_DUMP_FAILED:
	{
		SetWindowText(upload_label_hwnd, L"Memory dump saving failed.");
		EnableWindow(ok_button_hwnd, true);
		EnableWindow(cancel_button_hwnd, false);
		break;
	}	
	case CUSTOM_UPLOAD_STARTED:
	{
                SendMessage(progresss_bar_hwnd, PBM_SETRANGE32, 0, 100);
		SetWindowText(upload_label_hwnd, L"Initializing upload...");
		EnableWindow(ok_button_hwnd, false);
		EnableWindow(cancel_button_hwnd, false);
		break;
	}	
	case CUSTOM_UPLOAD_FINISHED:
	{
                swprintf(upload_progress_message, L"Finished. File: \"%s\".", file_name.c_str());
		SetWindowText(upload_label_hwnd, upload_progress_message);
		EnableWindow(ok_button_hwnd, true);
		EnableWindow(cancel_button_hwnd, false);
		break;
	}
	case CUSTOM_UPLOAD_CANCELED:
	{
                swprintf(upload_progress_message, L"Upload cancleled. You may share the file with our support manually.");
		SetWindowText(upload_label_hwnd, upload_progress_message);
		EnableWindow(ok_button_hwnd, true);
		EnableWindow(cancel_button_hwnd, false);
		break;
	}        
	case CUSTOM_UPLOAD_FAILED:
	{
		SetWindowText(upload_label_hwnd, L"Uploading failed. You may share the file with our support manually.");
		EnableWindow(ok_button_hwnd, true);
		EnableWindow(cancel_button_hwnd, false);
		break;
	}
	case WM_COMMAND:
	{
		if ((HWND)lParam == ok_button_hwnd)
		{
			EnableWindow(ok_button_hwnd, false);
			EnableWindow(cancel_button_hwnd, false);
			SetWindowText(upload_label_hwnd, L"");
			button_clicked = IDOK;
			upload_window_choose_variable.notify_one();
			break;
		}
		if ((HWND)lParam == cancel_button_hwnd)
		{
			EnableWindow(ok_button_hwnd, false);
			EnableWindow(cancel_button_hwnd, false);
			SetWindowText(upload_label_hwnd, L"");
			button_clicked = IDCANCEL;
			upload_window_choose_variable.notify_one();
			break;
		}
		break;
	}
	case CUSTOM_REQUESTED_CLICK: 
	{
		if (!upload_window_hwnd) { 
			button_clicked = IDOK;
			upload_window_choose_variable.notify_one();
		}
		break;
	}
	}

        return DefWindowProc(hwnd, msg, wParam, lParam);
}

UploadWindow::UploadWindow()
{
}

UploadWindow::~UploadWindow()
{
        if (window_thread)
        {
                PostMessage(upload_window_hwnd, CUSTOM_CLOSE_MSG, 0, 0);
                if (window_thread->joinable())
                        window_thread->join();
        }
};

UploadWindow *UploadWindow::getInstance()
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
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_NOCLOSE;
	wc.lpfnWndProc = FrameWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = CreateSolidBrush(RGB(23, 36, 45));
	wc.lpszMenuName = NULL;
	wc.lpszClassName = TEXT("uploaderwindow");

	if (!RegisterClassEx(&wc))
	{
		log_error << "Failed to create a class for uploader window " << GetLastError() << std::endl;
		upload_window_choose_variable.notify_one();
		return;
	}

	/* We only care about the main display */
	int screen_width = GetSystemMetrics(SM_CXSCREEN);
	int screen_height = GetSystemMetrics(SM_CYSCREEN);

	upload_window_hwnd = CreateWindowEx(
	    WS_EX_CLIENTEDGE,
	    TEXT("uploaderwindow"),
	    TEXT("Streamlabs OBS Crash Dump Uploader"),
	    WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU,
	    (screen_width - width) / 2, (screen_height - height) / 2,
	    width, height,
	    NULL, NULL, hInstance, this);
	if (!upload_window_hwnd)
	{
		log_error << "Failed to create an uploader window " << GetLastError() << std::endl;
		upload_window_choose_variable.notify_one();
		return;
	}

	int x_pos = 10;
	int y_pos = 10;
	int x_size = 470;
	int y_size = 150;
	RECT client_rect;
	if (GetClientRect(upload_window_hwnd, &client_rect))
	{
		x_size = client_rect.right - client_rect.left;
		y_size = client_rect.bottom - client_rect.top;
	}

	progresss_bar_hwnd = CreateWindow(
	    PROGRESS_CLASS, TEXT("ProgressWorker"),
	    WS_CHILD | PBS_SMOOTH,
	    x_pos, y_pos,
	    x_size - 20, 40,
	    upload_window_hwnd,
	    NULL, NULL, NULL);

	upload_label_hwnd = CreateWindow(
	    WC_STATIC, TEXT(""),
	    WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
	    x_pos, y_pos + 50,
	    x_size - 20, 40,
	    upload_window_hwnd,
	    NULL, NULL, NULL);

	ok_button_hwnd = CreateWindow(
	    WC_BUTTON, L"OK",
	    WS_TABSTOP | WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
	    x_size - 220, y_size - 50, 100, 40,
	    upload_window_hwnd,
	    NULL, NULL, NULL);

	cancel_button_hwnd = CreateWindow(
	    WC_BUTTON, L"Cancel",
	    WS_TABSTOP | WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
	    x_size - 110, y_size - 50, 100, 40,
	    upload_window_hwnd,
	    NULL, NULL, NULL);

	ShowWindow(upload_window_hwnd, SW_SHOWNORMAL);
	UpdateWindow(upload_window_hwnd);
	EnableWindow(ok_button_hwnd, false);
	EnableWindow(cancel_button_hwnd, false);

	upload_window_choose_variable.notify_one();
	MSG msg;

	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
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
	return;
}

void UploadWindow::uploadStarted()
{
	button_clicked = 0;
	PostMessage(upload_window_hwnd, CUSTOM_UPLOAD_STARTED, NULL, NULL);
	return;
}

void UploadWindow::uploadFailed()
{
	button_clicked = 0;
	PostMessage(upload_window_hwnd, CUSTOM_UPLOAD_FAILED, NULL, NULL);
	return;
}

void UploadWindow::uploadCanceled()
{
	button_clicked = 0;
	PostMessage(upload_window_hwnd, CUSTOM_UPLOAD_CANCELED, NULL, NULL);
	return;
}

void UploadWindow::savingFinished()
{
	button_clicked = 0;
	PostMessage(upload_window_hwnd, CUSTOM_SAVED_DUMP, NULL, NULL);
	return;
}

void UploadWindow::savingStarted()
{
	button_clicked = 0;
	PostMessage(upload_window_hwnd, CUSTOM_SAVE_STARTED, NULL, NULL);
	return;
}

void UploadWindow::savingFailed()
{
	button_clicked = 0;
	PostMessage(upload_window_hwnd, CUSTOM_SAVING_DUMP_FAILED, NULL, NULL);
	return;
}

void UploadWindow::crashCatched()
{
	button_clicked = 0;
	PostMessage(upload_window_hwnd, CUSTOM_CATCHED_CRASH, NULL, NULL);
	return;
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

bool UploadWindow::setDumpFileName(const std::wstring& new_file_name )
{
        file_name = new_file_name;
	return true;
}

bool UploadWindow::setTotalBytes(int new_total)
{
        total_bytes_to_send = new_total;
	return true;
}
 
bool UploadWindow::setUploadProgress(int sent_amount)
{
	bytes_sent = sent_amount;
	PostMessage(upload_window_hwnd, CUSTOM_PROGRESS_MSG, NULL, NULL);
	return true;
}
