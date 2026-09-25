#pragma once
#include "tsp.hpp"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <chrono>

// Synchronous Win32 rendering: never queue/drop proposal frames.
class Visualizer {
    HWND window = nullptr;
    const tsp::Instance& problem;
    tsp::Route current_route;
    tsp::Cost current = 0, best = 0;
    std::int64_t iteration = 0;
    int level = 0, delay;
    double temperature = 0;
    bool paused = false, step_once = false, accepted = false, finished = false;
    static constexpr const wchar_t* class_name = L"TspAnnealingViewer";
    static LRESULT CALLBACK procedure(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
        auto* self = reinterpret_cast<Visualizer*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (msg == WM_NCCREATE) {
            self = static_cast<Visualizer*>(reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams);
            self->window = hwnd;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        if (!self) return DefWindowProcW(hwnd, msg, w, l);
        switch (msg) {
        case WM_CLOSE: DestroyWindow(hwnd); return 0;
        case WM_DESTROY: self->window = nullptr; return 0;
        case WM_ERASEBKGND: return 1;
        case WM_SIZE: InvalidateRect(hwnd, nullptr, FALSE); return 0;
        case WM_GETMINMAXINFO:
            reinterpret_cast<MINMAXINFO*>(l)->ptMinTrackSize = {980,700};
            return 0;
        case WM_KEYDOWN:
            if (w == VK_ESCAPE || w == 'V') { DestroyWindow(hwnd); return 0; }
            if (w == VK_SPACE) self->paused = !self->paused;
            if (w == 'N') { self->paused = true; self->step_once = true; }
            if (w == VK_OEM_PLUS || w == VK_ADD) self->delay = std::min(1000, self->delay + 10);
            if (w == VK_OEM_MINUS || w == VK_SUBTRACT) self->delay = std::max(0, self->delay - 10);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        case WM_PAINT: self->paint(); return 0;
        default: return DefWindowProcW(hwnd, msg, w, l);
        }
    }
    void paint() {
        PAINTSTRUCT ps;
        HDC screen = BeginPaint(window, &ps);
        RECT rect;
        GetClientRect(window, &rect);
        const int width = std::max(1L, rect.right), height = std::max(1L, rect.bottom);
        HDC dc = CreateCompatibleDC(screen);
        HBITMAP bitmap = CreateCompatibleBitmap(screen, width, height);
        auto old_bitmap = SelectObject(dc, bitmap);
        HBRUSH background = CreateSolidBrush(RGB(245,248,251));
        FillRect(dc, &rect, background);
        DeleteObject(background);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, RGB(28,55,77));
        HFONT font = CreateFontW(19,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
                                OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,
                                DEFAULT_PITCH,L"Segoe UI");
        auto old_font = SelectObject(dc,font);
        auto text = [&](int x,int y,const std::wstring& s) { TextOutW(dc,x,y,s.c_str(),static_cast<int>(s.size())); };
        const std::wstring name(problem.name.begin(),problem.name.end());
        text(28,18,name + (finished ? L" | Final best tour" : L" | Current search tour (every proposal)"));
        text(28,48,L"Iteration: " + std::to_wstring(iteration) + L"    Level: " + std::to_wstring(level) +
             L"    T: " + std::to_wstring(temperature));
        text(28,76,L"Current: " + std::to_wstring(current) + L"    Best: " + std::to_wstring(best) +
             (finished ? L"    DONE" : (accepted ? L"    ACCEPTED" : L"    REJECTED / INITIAL")));
        double minx=1e100, maxx=-1e100, miny=1e100, maxy=-1e100;
        for (auto p:problem.points) { minx=std::min(minx,p.x); maxx=std::max(maxx,p.x); miny=std::min(miny,p.y); maxy=std::max(maxy,p.y); }
        const double scale=std::max(0.001,std::min((width-110)/std::max(1.0,maxx-minx),
                                                  (height-225)/std::max(1.0,maxy-miny)));
        const double left=(width-(maxx-minx)*scale)/2;
        auto position=[&](int i) -> POINT { auto p=problem.points[i];
            return {LONG(left+(p.x-minx)*scale),LONG(135+(maxy-p.y)*scale)}; };
        HPEN pen=CreatePen(PS_SOLID,2,RGB(27,126,163));
        auto old_pen=SelectObject(dc,pen);
        if (!current_route.empty()) {
            std::vector<POINT> path;
            for (int i:current_route) path.push_back(position(i));
            path.push_back(path.front());
            Polyline(dc,path.data(),static_cast<int>(path.size()));
        }
        SelectObject(dc,old_pen); DeleteObject(pen);
        HBRUSH city=CreateSolidBrush(RGB(28,55,77));
        auto old_brush=SelectObject(dc,city);
        for (int i=0;i<static_cast<int>(problem.points.size());++i) {
            POINT q=position(i);
            Ellipse(dc,q.x-4,q.y-4,q.x+4,q.y+4);
            int dx=5,dy=-16;
            if(problem.name=="att48") {
                switch(i+1) {
                case 6: dx=-23; dy=2; break;
                case 30: dx=-27; dy=-15; break;
                case 36: dx=-27; dy=-20; break;
                case 18: dy=1; break;
                case 37: dy=1; break;
                default: break;
                }
            }
            SetBkColor(dc,RGB(245,248,251)); SetBkMode(dc,OPAQUE);
            text(q.x+dx,q.y+dy,std::to_wstring(i+1));
            SetBkMode(dc,TRANSPARENT);
        }
        SelectObject(dc,old_brush); DeleteObject(city);
        SetTextColor(dc,RGB(85,100,112));
        text(28,height-70,L"Space: pause/resume | N: next step | +/-: delay ("+std::to_wstring(delay)+L" ms)");
        text(28,height-42,L"Esc / V / close: disable visualization and continue solving" + std::wstring(paused ? L"  [PAUSED]" : L""));
        BitBlt(screen,0,0,width,height,dc,0,0,SRCCOPY);
        SelectObject(dc,old_font); DeleteObject(font);
        SelectObject(dc,old_bitmap); DeleteObject(bitmap); DeleteDC(dc);
        EndPaint(window,&ps);
        GdiFlush();
    }
    void events() {
        MSG msg;
        while (PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)) {
            TranslateMessage(&msg); DispatchMessageW(&msg);
        }
    }
public:
    explicit Visualizer(const tsp::Instance& p, int frame_ms) : problem(p), delay(frame_ms) {
        WNDCLASSW wc{};
        wc.lpfnWndProc=procedure; wc.hInstance=GetModuleHandleW(nullptr);
        wc.lpszClassName=class_name; wc.hCursor=LoadCursor(nullptr,IDC_ARROW);
        if (!RegisterClassW(&wc) && GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)
            throw std::runtime_error("Cannot register visualization window");
        if (!CreateWindowW(class_name,L"TSP - Simulated Annealing",WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT,CW_USEDEFAULT,1120,820,nullptr,nullptr,wc.hInstance,this))
            throw std::runtime_error("Cannot create visualization window");
        ShowWindow(window,SW_SHOW);
    }
    ~Visualizer() { if(window) DestroyWindow(window); }
    bool update(const tsp::Snapshot& s) {
        events();
        if (!window) return false;
        current_route=s.route; current=s.current; best=s.best; iteration=s.iteration;
        level=s.level; temperature=s.temperature; accepted=s.accepted;
        // Force this exact state to be painted before the next proposal is generated.
        RedrawWindow(window,nullptr,nullptr,RDW_INVALIDATE|RDW_UPDATENOW);
        const auto until=std::chrono::steady_clock::now()+std::chrono::milliseconds(delay);
        while (window && ((paused && !step_once) || std::chrono::steady_clock::now()<until)) {
            events();
            if (window) MsgWaitForMultipleObjects(0,nullptr,FALSE,5,QS_ALLINPUT);
        }
        step_once=false;
        return window!=nullptr;
    }
    void show_result(const tsp::Result& r) {
        if(!window) return;
        finished=true; current_route=r.route; current=best=r.best;
        RedrawWindow(window,nullptr,nullptr,RDW_INVALIDATE|RDW_UPDATENOW);
        while(window) { events(); if(window) MsgWaitForMultipleObjects(0,nullptr,FALSE,50,QS_ALLINPUT); }
    }
};
#else
class Visualizer {
public:
    Visualizer(const tsp::Instance&,int) { throw std::runtime_error("Live visualization requires Windows; use --no-visual"); }
    bool update(const tsp::Snapshot&) { return false; }
    void show_result(const tsp::Result&) {}
};
#endif
