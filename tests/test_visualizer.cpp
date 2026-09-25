// Windows-only integration test of this program's own GUI. No other app is touched.
#include "../src/visualizer.hpp"
#include <iostream>
#include <filesystem>
#include <thread>

static HWND own_window() {
    HWND found=nullptr;
    EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM target)->BOOL {
        wchar_t name[100]; GetClassNameW(w,name,100);
        if(std::wstring(name)==L"TspAnnealingViewer") {
            *reinterpret_cast<HWND*>(target)=w; return FALSE;
        }
        return TRUE;
    },reinterpret_cast<LPARAM>(&found));
    if(!found) throw std::runtime_error("Test window not found");
    return found;
}
static void capture(HWND w,const char* path) {
    RECT r; GetClientRect(w,&r);
    HDC dc=GetDC(w), memory=CreateCompatibleDC(dc);
    HBITMAP bitmap=CreateCompatibleBitmap(dc,r.right,r.bottom);
    auto old=SelectObject(memory,bitmap);
    BitBlt(memory,0,0,r.right,r.bottom,dc,0,0,SRCCOPY);
    SelectObject(memory,old);
    BITMAPINFO info{};
    info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth=r.right; info.bmiHeader.biHeight=-r.bottom;
    info.bmiHeader.biPlanes=1; info.bmiHeader.biBitCount=32;
    std::vector<char> pixels(static_cast<std::size_t>(r.right)*r.bottom*4);
    if(!GetDIBits(dc,bitmap,0,r.bottom,pixels.data(),&info,DIB_RGB_COLORS))
        throw std::runtime_error("Cannot capture test frame");
    BITMAPFILEHEADER header{};
    header.bfType=0x4d42; header.bfOffBits=sizeof(header)+sizeof(info.bmiHeader);
    header.bfSize=header.bfOffBits+static_cast<DWORD>(pixels.size());
    std::ofstream file(path,std::ios::binary);
    file.write(reinterpret_cast<char*>(&header),sizeof(header));
    file.write(reinterpret_cast<char*>(&info.bmiHeader),sizeof(info.bmiHeader));
    file.write(pixels.data(),pixels.size());
    DeleteObject(bitmap); DeleteDC(memory); ReleaseDC(w,dc);
}
int main() {
    try {
        std::filesystem::create_directories("tmp");
        const auto p=tsp::read_instance("data/att48.tsp");
        tsp::Config c; c.steps=100; c.max_levels=2;
        const auto base=tsp::solve(p,c);
        for(int mode=0;mode<3;++mode) {
            Visualizer viewer(p,0);
            HWND window=own_window();
            int frames=0;
            const auto result=tsp::solve(p,c,[&](const tsp::Snapshot& s) {
                ++frames;
                if(s.iteration==4) {
                    // Space pauses; N releases exactly one proposal; Escape/V/close must
                    // still be processed in the paused event loop.
                    PostMessageW(window,WM_KEYDOWN,VK_SPACE,0);
                    std::thread input([window] {
                        std::this_thread::sleep_for(std::chrono::milliseconds(60));
                        PostMessageW(window,WM_KEYDOWN,'N',0);
                    });
                    bool active=viewer.update(s);
                    input.join();
                    if(!active) throw std::runtime_error("Single step unexpectedly closed window");
                    return true;
                }
                if(s.iteration==5) {
                    capture(window,"tmp/visualizer_frame.bmp");
                    std::thread input([window,mode] {
                        std::this_thread::sleep_for(std::chrono::milliseconds(60));
                        if(mode==0) PostMessageW(window,WM_CLOSE,0,0);
                        else PostMessageW(window,WM_KEYDOWN,mode==1 ? VK_ESCAPE : 'V',0);
                    });
                    bool active=viewer.update(s);
                    input.join();
                    return active;
                }
                return viewer.update(s);
            });
            if(frames!=6 || result.proposals!=200 || result.route!=base.route || result.accepted!=base.accepted)
                throw std::runtime_error("Closing UI changed search or failed to detach");
        }
        std::cout << "PASS: real Win32 pause, single step, X/Escape/V close while paused, search continues identically\n";
        return 0;
    } catch(const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
