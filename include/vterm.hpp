#pragma once

#include <algorithm>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <wchar.h>
#include <cstring>
#include <deque>
#include <vector>
#include <string_view>
#include "signal_slot.hpp"

namespace nstd::vterm
{
constexpr const wchar_t VTRM_INVALID_CHAR = (wchar_t)0xfffd;
constexpr const char VTRM_KEY_UP[] =        "\033[A";
constexpr const char VTRM_KEY_DOWN[] =      "\033[B";
constexpr const char VTRM_KEY_RIGHT[] =     "\033[C";
constexpr const char VTRM_KEY_LEFT[] =      "\033[D";
constexpr const char VTRM_KEY_HOME[] =      "\033[H";
constexpr const char VTRM_KEY_END[] =       "\033[Y";
constexpr const char VTRM_KEY_INSERT[] =    "\033[L";
constexpr const char VTRM_KEY_BACKSPACE[] = "\x08";
constexpr const char VTRM_KEY_ESCAPE[] =    "\x1b";
constexpr const char VTRM_KEY_BACK_TAB[] =  "\033[Z";
constexpr const char VTRM_KEY_PAGE_UP[] =   "\033[V";
constexpr const char VTRM_KEY_PAGE_DOWN[] = "\033[U";
constexpr const char VTRM_KEY_F1[] =        "\033OP";
constexpr const char VTRM_KEY_F2[] =        "\033OQ";
constexpr const char VTRM_KEY_F3[] =        "\033OR";
constexpr const char VTRM_KEY_F4[] =        "\033OS";
constexpr const char VTRM_KEY_F5[] =        "\033OT";
constexpr const char VTRM_KEY_F6[] =        "\033OU";
constexpr const char VTRM_KEY_F7[] =        "\033OV";
constexpr const char VTRM_KEY_F8[] =        "\033OW";
constexpr const char VTRM_KEY_F9[] =        "\033OX";
constexpr const char VTRM_KEY_F10[] =       "\033OY";
constexpr const uint32_t BUF_MAX = 100;
constexpr const uint32_t PAR_MAX = 8;
constexpr const uint32_t TAB = 8;

enum class vtrm_color_t : int32_t
{
    VTRM_COLOR_DEFAULT = -1,
    VTRM_COLOR_BLACK = 1,
    VTRM_COLOR_RED,
    VTRM_COLOR_GREEN,
    VTRM_COLOR_YELLOW,
    VTRM_COLOR_BLUE,
    VTRM_COLOR_MAGENTA,
    VTRM_COLOR_CYAN,
    VTRM_COLOR_WHITE,
    VTRM_COLOR_MAX
};

struct vtrm_attrs
{
    bool bold { false };
    bool dim { false };
    bool underline { false };
    bool blink { false };
    bool reverse { false };
    bool invisible { false };
    vtrm_color_t fg { vtrm_color_t::VTRM_COLOR_DEFAULT };
    vtrm_color_t bg { vtrm_color_t::VTRM_COLOR_DEFAULT };
};

struct vtrm_char
{
    wchar_t c { L' '};
    vtrm_attrs a {};
};

struct vtrm_point
{
    size_t r { 0 };
    size_t c { 0 };

    bool operator ==(const vtrm_point& p)
    {
        return r == p.r && c == p.c;
    }

    bool operator !=(const vtrm_point& p)
    {
        return !(*this == p);
    }
};

struct vtrm_line
{
    bool dirty { false };
    std::vector<vtrm_char> chars{};
};

struct vtrm_screen
{
    size_t nline{ 0 };
    size_t ncol{ 0 };

    std::deque<vtrm_line> lines{};
};

class vterm
{
public:

    vterm() = default;
    vterm(const vterm&) = default;
    vterm(vterm&&) = default;

    nstd::signal_slot::signal<vterm*, const void*> moved_signal,
                                                update_signal,
                                                answer_signal,
                                                bell_signal,
                                                cursor_signal,
                                                scrolled_up_signal,
                                                scrolled_down_signal;

    bool create(size_t nline, size_t ncol, const std::vector<wchar_t>& acs = {})
    {
        if (!nline || !ncol) return false;

        /* ASCII-safe defaults for box-drawing characters. */
        static const std::vector<wchar_t> default_acschars{ L'>', L'<', L'^', L'v', L'#', L'+', L':', L'o', L'#', L'#', L'+', L'+', L'+', L'+', L'+', L'~', L'-', L'-', L'-', L'_', L'+', L'+', L'+', L'+', L'|', L'<', L'>', L'*', L'!', L'f', L'o' };

        acschars = std::size(acs) ? acs : default_acschars;

        return vtrm_resize(nline, ncol);
    }

    void write(std::string_view s)
    {
        vtrm_point oc = curs;

        for (auto& c : s)
        {
            if (handle_char(c)) continue;
            else if (acs) write_char_at_curs(tacs((unsigned char)c));
            else if (nmb >= BUF_MAX) write_char_at_curs(get_mb_char());
            else
            {
                switch (test_mb_char())
                {
                case (size_t)-1: write_char_at_curs(get_mb_char()); break;
                case (size_t)-2: mb[nmb++] = c;                     break;
                }

                if (test_mb_char() <= MB_LEN_MAX) write_char_at_curs(get_mb_char());
            }
        }

        notify(dirty, oc != curs);
    }

    vtrm_screen& get_screen()
    {
        return screen;
    }

    vtrm_point& get_cursor()
    {
        return curs;
    }

    void clean()
    {
        dirty = false;

        for (auto& l : screen.lines) l.dirty = false;
    }

    void reset()
    {
        curs.r = curs.c = oldcurs.r = oldcurs.c = acs = 0;
        reset_parser();
        attrs = oldattrs = vtrm_attrs{};
        std::memset(&ms, 0, sizeof(ms));
        clear_lines(0, screen.nline);
        cursor_signal.emit(this, "t");
        notify(true, true);
    }

private:

    vtrm_point curs{}, oldcurs{};
    vtrm_attrs attrs{}, oldattrs{};

    bool dirty{ false }, acs{ false }, ignored{ false };
    vtrm_screen screen{};
    vtrm_line tabs{};

    std::vector<wchar_t> acschars{};

    mbstate_t ms{};
    size_t nmb{ 0 };
    char mb[BUF_MAX + 1] = { 0 };

    size_t pars[PAR_MAX] = { 0 };   
    size_t npar{ 0 };
    size_t arg{ 0 };
    enum { S_NUL, S_ESC, S_ARG } state;

    wchar_t tacs(unsigned char c)
    {
        /* The terminfo alternate character set for ANSI. */
        static unsigned char map[] = { 0020U, 0021U, 0030U, 0031U, 0333U, 0004U,
                                       0261U, 0370U, 0361U, 0260U, 0331U, 0277U,
                                       0332U, 0300U, 0305U, 0176U, 0304U, 0304U,
                                       0304U, 0137U, 0303U, 0264U, 0301U, 0302U,
                                       0263U, 0363U, 0362U, 0343U, 0330U, 0234U,
                                       0376U };
        
        for (size_t i { 0 }; i < sizeof(map); ++i) if (map[i] == c) return acschars[i];

        return (wchar_t)c;
    }

    void dirty_lines(size_t s, size_t e)
    {
        dirty = true;

        for (size_t i = s; i < e; ++i) screen.lines[i].dirty = true;
    }

    void clear_line(vtrm_line &l, size_t s, size_t e)
    {
        dirty = l.dirty = true;
        
        std::fill(std::begin(l.chars) + s, (e < screen.ncol) ? std::begin(l.chars) + e : std::end(l.chars), vtrm_char{});
    }

    void clear_lines(size_t r, size_t n)
    {
        for (size_t i = r; i < r + n && i < screen.nline; ++i) clear_line(screen.lines[i], 0, screen.ncol);
    }

    void scrup(size_t r, size_t n)
    {
        n = std::min(n, screen.nline - 1 - r);

        if (n)
        {
            for (size_t i { 0 }; i < n; ++i)
            {
                screen.lines.erase(std::begin(screen.lines) + r);
                screen.lines.emplace_back();
                screen.lines.back().chars.resize(screen.ncol);
            }

            clear_lines(screen.nline - n, n);
            dirty_lines(r, screen.nline);

            scrolled_up_signal.emit(this, nullptr);
        }
    }

    void scrdn(size_t r, size_t n)
    {
        n = std::min(n, screen.nline - 1 - r);

        if (n)
        {
            for (size_t i { 0 }; i < n; ++i)
            {
                screen.lines.erase(std::end(screen.lines) - r);
                screen.lines.emplace_front();
                screen.lines.front().chars.resize(screen.ncol);
            }

            clear_lines(r, n);
            dirty_lines(r, screen.nline);

            scrolled_down_signal.emit(this, nullptr);
        }
    }

    vtrm_line &current_line()
    {
        return screen.lines[std::min(curs.r, screen.nline - 1)];
    }

    void ed()
    {
        auto &l { current_line() };
        size_t b { 0 }, e { screen.nline };

        switch (pars[0])
        {
            case 0: b = curs.r + 1; clear_line(l, curs.c, screen.ncol); break;
            case 1: e = curs.r - 1; clear_line(l, 0, curs.c);           break;
            case 2:  /* use defaults */                                 break;
            default: /* do nothing   */                                 return;
        }

        clear_lines(b, e - b);
    }

    void ich()
    {
        auto &l { current_line() };
        size_t n = pars[0] ? pars[0] : 1;

        if (n > screen.ncol - curs.c - 1) n = screen.ncol - curs.c - 1;

        std::memmove(std::data(l.chars) + curs.c + n, std::data(l.chars) + curs.c, std::min(screen.ncol - 1 - curs.c, (screen.ncol - curs.c - n - 1)) * sizeof(vtrm_char));
        
        clear_line(l, curs.c, n);
    }

    void dch()
    {
        auto &l { current_line() };
        size_t n = pars[0] ? pars[0] : 1;
        
        if (n > screen.ncol - curs.c) n = screen.ncol - curs.c;
        else if (n == 0) return;

        std::memmove(std::data(l.chars) + curs.c, std::data(l.chars) + curs.c + n, (screen.ncol - curs.c - n) * sizeof(vtrm_char));

        clear_line(l, screen.ncol - n, screen.ncol);
        /* VT102 manual says the attribute for the newly empty characters
         * should be the same as the last character moved left, which isn't
         * what clear_line() currently does.
         */
    }

    void el()
    {
        auto &l { current_line() };

        switch (pars[0])
        {
            case 0: clear_line(l, curs.c, screen.ncol);                      break;
            case 1: clear_line(l, 0, std::min(curs.c + 1, screen.ncol - 1)); break;
            case 2: clear_line(l, 0, screen.ncol);                           break;
        }
    }

    void sgr()
    {
        for (size_t i = 0; i < npar; ++i)
        {
            switch (pars[i])
            {
                case  0: attrs                    = vtrm_attrs{};     break;
                case  1: case 22: attrs.bold      = pars[0] < 20;     break;
                case  2: case 23: attrs.dim       = pars[0] < 20;     break;
                case  4: case 24: attrs.underline = pars[0] < 20;     break;
                case  5: case 25: attrs.blink     = pars[0] < 20;     break;
                case  7: case 27: attrs.reverse   = pars[0] < 20;     break;
                case  8: case 28: attrs.invisible = pars[0] < 20;     break;
                case 10: case 11: acs             = pars[0] > 10;     break;
                case 30: attrs.fg = vtrm_color_t::VTRM_COLOR_BLACK;   break;
                case 31: attrs.fg = vtrm_color_t::VTRM_COLOR_RED;     break;
                case 32: attrs.fg = vtrm_color_t::VTRM_COLOR_GREEN;   break;
                case 33: attrs.fg = vtrm_color_t::VTRM_COLOR_YELLOW;  break;
                case 34: attrs.fg = vtrm_color_t::VTRM_COLOR_BLUE;    break;
                case 35: attrs.fg = vtrm_color_t::VTRM_COLOR_MAGENTA; break;
                case 36: attrs.fg = vtrm_color_t::VTRM_COLOR_CYAN;    break;
                case 37: attrs.fg = vtrm_color_t::VTRM_COLOR_WHITE;   break;
                case 39: attrs.fg = vtrm_color_t::VTRM_COLOR_DEFAULT; break;
                case 40: attrs.bg = vtrm_color_t::VTRM_COLOR_BLACK;   break;
                case 41: attrs.bg = vtrm_color_t::VTRM_COLOR_RED;     break;
                case 42: attrs.bg = vtrm_color_t::VTRM_COLOR_GREEN;   break;
                case 43: attrs.bg = vtrm_color_t::VTRM_COLOR_YELLOW;  break;
                case 44: attrs.bg = vtrm_color_t::VTRM_COLOR_BLUE;    break;
                case 45: attrs.bg = vtrm_color_t::VTRM_COLOR_MAGENTA; break;
                case 46: attrs.bg = vtrm_color_t::VTRM_COLOR_CYAN;    break;
                case 47: attrs.bg = vtrm_color_t::VTRM_COLOR_WHITE;   break;
                case 49: attrs.bg = vtrm_color_t::VTRM_COLOR_DEFAULT; break;
            }
        }
    }

    void rep()
    {
        if (!curs.c) return;

        auto &l { current_line() };
        wchar_t r { l.chars[curs.c - 1].c };

        for (size_t i = 0; i < (pars[0] ? pars[0] : 1); ++i) write_char_at_curs(r);
    }

    void dsr()
    {
        char answer_data[BUF_MAX + 1] = { 0 };

        std::memset(answer_data, 0, sizeof(answer_data));
        std::snprintf(answer_data, BUF_MAX, "\033[%zd;%zdR", curs.r + 1, curs.c + 1);

        answer_signal.emit(this, &answer_data);
    }

    void reset_parser()
    {
        std::memset(pars, 0, sizeof(pars));
        
        npar = arg = ignored = 0;
        state = S_NUL;
    }

    void consume_arg()
    {
        if (npar < PAR_MAX) pars[npar++] = arg;
        
        arg = 0;
    }

    void fix_cursor()
    {
        curs.r = std::min(curs.r, screen.nline - 1);
        curs.c = std::min(curs.c, screen.ncol - 1);
    }

    bool handle_char(char i)
    {
        auto &l { current_line() };
        const size_t par1 { (pars[0] ? pars[0] : 1) };
        char cs[] = { i, 0 };

        #define ON(S, C, A) if (state == (S) && std::strchr(C, i)){ A; return true; }
        #define DO(S, C, A) ON(S, C, consume_arg(); if (!ignored) {A;} fix_cursor(); reset_parser(););

        DO(S_NUL, "\x07",       bell_signal.emit(this, nullptr))
        DO(S_NUL, "\x08",       if (curs.c) curs.c--)
        DO(S_NUL, "\x09",       while (++curs.c < screen.ncol - 1 && tabs.chars[curs.c].c != L'*'))
        DO(S_NUL, "\x0a",       curs.r < screen.nline - 1 ? (void)curs.r++ : scrup(0, 1))
        DO(S_NUL, "\x0d",       curs.c = 0)
        ON(S_NUL, "\x1b",       state = S_ESC)
        ON(S_ESC, "\x1b",       state = S_ESC)
        DO(S_ESC, "H",          tabs.chars[curs.c].c = L'*')
        DO(S_ESC, "7",          oldcurs = curs; oldattrs = attrs)
        DO(S_ESC, "8",          curs = oldcurs; attrs = oldattrs)
        ON(S_ESC, "+*()",       ignored = true; state = S_ARG)
        DO(S_ESC, "c",          reset())
        ON(S_ESC, "[",          state = S_ARG)
        ON(S_ARG, "\x1b",       state = S_ESC)
        ON(S_ARG, ";",          consume_arg())
        ON(S_ARG, "?",          (void)0)
        ON(S_ARG, "0123456789", arg = arg * 10 + std::atoi(cs))
        DO(S_ARG, "A",          curs.r = std::max(curs.r - par1, (size_t)0))
        DO(S_ARG, "B",          curs.r = std::min(curs.r + par1, screen.nline - 1))
        DO(S_ARG, "C",          curs.c = std::min(curs.c + par1, screen.ncol - 1))
        DO(S_ARG, "D",          curs.c = std::min(curs.c - par1, curs.c))
        DO(S_ARG, "E",          curs.c = 0; curs.r = std::min(curs.r + par1, screen.nline - 1))
        DO(S_ARG, "F",          curs.c = 0; curs.r = std::max(curs.r - par1, (size_t)0))
        DO(S_ARG, "G",          curs.c = std::min(par1 - 1, screen.ncol - 1))
        DO(S_ARG, "d",          curs.r = std::min(par1 - 1, screen.nline - 1))
        DO(S_ARG, "Hf",         curs.r = par1 - 1; curs.c = par1 - 1)
        DO(S_ARG, "I",          while (++curs.c < screen.ncol - 1 && tabs.chars[curs.c].c != L'*'))
        DO(S_ARG, "J",          ed())
        DO(S_ARG, "K",          el())
        DO(S_ARG, "L",          scrdn(curs.r, par1))
        DO(S_ARG, "M",          scrup(curs.r, par1))
        DO(S_ARG, "P",          dch())
        DO(S_ARG, "S",          scrup(0, par1))
        DO(S_ARG, "T",          scrdn(0, par1))
        DO(S_ARG, "X",          clear_line(l, curs.c, par1))
        DO(S_ARG, "Z",          while (curs.c && tabs.chars[--curs.c].c != L'*'))
        DO(S_ARG, "b",          rep());
        DO(S_ARG, "c",          answer_signal.emit(this, "\033[?6c"))
        DO(S_ARG, "g",          if (pars[0] == 3) clear_line(tabs, 0, screen.ncol))
        DO(S_ARG, "m",          sgr())
        DO(S_ARG, "n",          if (pars[0] == 6) dsr())
        DO(S_ARG, "h",          if (pars[0] == 25) cursor_signal.emit(this, "t"))
        DO(S_ARG, "i",          (void)0)
        DO(S_ARG, "l",          if (pars[0] == 25) cursor_signal.emit(this, "f"))
        DO(S_ARG, "s",          oldcurs = curs; oldattrs = attrs)
        DO(S_ARG, "u",          curs = oldcurs; attrs = oldattrs)
        DO(S_ARG, "@",          ich())

        return reset_parser(), false;
    }

    void notify(bool update, bool moved)
    {
        if (update) update_signal.emit(this, &screen);
        if (moved) moved_signal.emit(this, &curs);
    }

    bool vtrm_resize(size_t nline, size_t ncol)
    {
        if (nline < 2 || ncol < 2) return false;

        screen.lines.resize(nline);
        for (auto &l : screen.lines) l.chars.resize(ncol);
        
        tabs.chars.resize(ncol);
        tabs.chars[0].c = tabs.chars[ncol - 1].c = L'*';
        for (size_t i { 0 }; i < ncol; ++i) if (i % TAB == 0) tabs.chars[i].c = L'*';

        screen.nline = nline;
        screen.ncol = ncol;

        fix_cursor();
        dirty_lines(0, nline);
        notify(true, true);

        return true;
    }

    void write_char_at_curs(wchar_t w)
    {
        #ifdef TMT_HAS_WCWIDTH
        extern int wcwidth(wchar_t c);
        if (wcwidth(w) > 1)  w = TMT_INVALID_CHAR;
        if (wcwidth(w) < 0) return;
        #endif

        auto &l { current_line() };

        l.chars[curs.c].c = w;
        l.chars[curs.c].a = attrs;
        l.dirty = dirty = true;

        if (curs.c < screen.ncol - 1) ++curs.c;
        else
        {
            curs.c = 0;
            ++curs.r;
        }

        if (curs.r >= screen.nline)
        {
            curs.r = screen.nline - 1;
            scrup(0, 1);
        }
    }

    size_t test_mb_char()
    {
        mbstate_t ts { ms };

        return nmb ? mbrtowc(nullptr, mb, nmb, &ts) : (size_t)-2;
    }

    wchar_t get_mb_char()
    {
        wchar_t c { 0 };
        size_t n { mbrtowc(&c, mb, nmb, &ms) };
        
        nmb = 0;
        
        return (n == (size_t)-1 || n == (size_t)-2) ? VTRM_INVALID_CHAR : c;
    }
};

}
