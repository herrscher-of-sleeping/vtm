// Copyright (c) NetXS Group.
// Licensed under the MIT license.

#ifndef NETXS_RICHTEXT_HPP
#define NETXS_RICHTEXT_HPP

#include "ansi.hpp"
#include "../text/logger.hpp"
#include "../abstract/ring.hpp"

#include <span>

namespace netxs::console
{
    using namespace netxs::ui::atoms;
    using namespace std::literals;

    using ansi::qiew;
    using ansi::writ;
    using ansi::deco;
    using irgb = netxs::ui::atoms::irgb<ui32>;

    class poly
    {
        cell grade[256];

    public:
        poly() = default;

        poly(cell const& basis)
        {
            recalc(basis);
        }

        void recalc(cell const& basis)
        {
            for (auto k = 0; k < 256; k++)
            {
                auto& b = grade[k];

                b = basis;
                b.bga(b.bga() * k >> 8);
                b.fga(b.fga() * k >> 8);
            }
        }

        cell const& operator [] (uint8_t k) const
        {
            return grade[k];
        }
    };

    // richtext: Canvas core grid.
    class core
    {
        // Prefill canvas using brush.
        core(twod const& coor, twod const& size, cell const& brush)
            : region{ coor  , size },
              client{ dot_00, size },
              canvas(size.x * size.y, brush),
              marker{ brush }
        { }
        // Prefill canvas using zero.
        core(twod const& coor, twod const& size)
            : region{ coor  , size },
              client{ dot_00, size },
              canvas(size.x * size.y)
        { }

    protected:
        si32 digest = 0;
        cell marker;
        rect region;
        grid canvas;
        rect client;

    public:
        using span = std::span<cell const>;

        core()                         = default;
        core(core&&)                   = default;
        core(core const&)              = default;
        core& operator = (core&&)      = default;
        core& operator = (core const&) = default;
        core(span const& body, twod const& size)
            : region{ dot_00, size },
              client{ dot_00, size },
              canvas( body.begin(), body.end() )
        {
            assert(size.x * size.y == std::distance(body.begin(), body.end()));
        }
        core(cell const& fill, si32 length)
            : region{ dot_00, twod{ length, 1 } },
              client{ dot_00, twod{ length, 1 } },
              canvas( length, fill )
        { }

        friend void swap(core& lhs, core& rhs)
        {
            std::swap(lhs.digest, rhs.digest);
            std::swap(lhs.marker, rhs.marker);
            std::swap(lhs.region, rhs.region);
            std::swap(lhs.canvas, rhs.canvas);
            std::swap(lhs.client, rhs.client);
        }
        constexpr auto& size() const        { return region.size;        }
        auto& coor() const                  { return region.coor;        }
        auto& area() const                  { return region;             }
        auto  hash()                        { return digest;             } // core: Return the digest value that associatated with the current canvas size.
        auto  data() const                  { return canvas.data();      }
        auto  data()                        { return canvas.data();      }
        auto& pick()                        { return canvas;             }
        auto  iter()                        { return canvas.begin();     }
        auto  iend()                        { return canvas.end();       }
        auto  iter() const                  { return canvas.begin();     }
        auto  iend() const                  { return canvas.end();       }
        auto  test(twod const& coord) const { return region.size.inside(coord); } // core: Check the coor inside the canvas.
        auto  data(twod const& coord)       { return  data() + coord.x + coord.y * region.size.x; } // core: Return the offset of the cell corresponding to the specified coordinates.
        auto  data(twod const& coord) const { return  data() + coord.x + coord.y * region.size.x; } // core: Return the const offset value of the cell.
        auto& data(size_t offset)           { return*(data() + offset);  } // core: Return the const offset value of the cell corresponding to the specified coordinates.
        auto& operator [] (twod const& c)   { return*(data(c));          } // core: Return reference of the canvas cell at the specified location. It is dangerous in case of layer resizing.
        auto& mark()                        { return marker;             } // core: Return a reference to the default cell value.
        auto& mark() const                  { return marker;             } // core: Return a reference to the default cell value.
        auto& mark(cell const& c)           { marker = c; return marker; } // core: Set the default cell value.
        void  move(twod const& newcoor)     { region.coor = newcoor;     } // core: Change the location of the face.
        void  step(twod const& delta)       { region.coor += delta;      } // core: Shift location of the face by delta.
        void  back(twod const& delta)       { region.coor -= delta;      } // core: Shift location of the face by -delta.
        void  link(id_t id)                 { marker.link(id);           } // core: Set the default object ID.
        auto  link(twod const& coord) const { return test(coord) ? (*(data(coord))).link() : 0; } // core: Return ID of the object in cell at the specified coordinates.
        auto  view() const                  { return client;    }
        void  view(rect const& viewreg)     { client = viewreg; }
        void  size(twod const& newsize) // core: Change the size of the face.
        {
            if (region.size(std::max(dot_00, newsize)))
            {
                client.size = region.size;
                digest++;
                canvas.assign(region.size.x * region.size.y, marker);
            }
        }
        void crop(si32 newsizex, cell const& c = {}) // core: Resize while saving the textline.
        {
            region.size.x = newsizex;
            region.size.y = 1;
            client.size = region.size;
            canvas.resize(newsizex, c);
            digest++;
        }
        //todo unify
        template<bool BOTTOM_ANCHORED = faux>
        void crop(twod const& newsize) // core: Resize while saving the bitmap.
        {
            core block{ region.coor, newsize };
            if constexpr (BOTTOM_ANCHORED) block.step({ 0, region.size.y - newsize.y });

            netxs::onbody(block, *this, cell::shaders::full);
            region.size = newsize;
            client.size = region.size;
            swap(block);
            digest++;
        }
        template<bool BOTTOM_ANCHORED = faux>
        void crop(twod const& newsize, cell const& c) // core: Resize while saving the bitmap.
        {
            core block{ region.coor, newsize, c };
            if constexpr (BOTTOM_ANCHORED) block.step({ 0, region.size.y - newsize.y });
                
            netxs::onbody(block, *this, cell::shaders::full);
            region.size = newsize;
            client.size = region.size;
            swap(block);
            digest++;
        }
        void kill() // core: Collapse canvas to size zero (see para).
        {
            region.size.x = 0;
            client.size.x = 0;
            canvas.resize(0);
            digest++;
        }
        void wipe(cell const& c) { std::fill(canvas.begin(), canvas.end(), c); } // core: Fill canvas with specified marker.
        void wipe()              { wipe(marker); } // core: Fill canvas with default color.
        void wipe(id_t id)                         // core: Fill canvas with specified id.
        {
            auto my = marker.link();
            marker.link(id);
            wipe(marker);
            marker.link(my);
        }
        template<class P>
        auto each(P proc) // core: Exec a proc for each cell.
        {
            using ret_t = std::invoke_result_t<P, cell&>;
            static constexpr auto plain = std::is_same_v<void, ret_t>;

            for (auto& c : canvas)
            {
                if constexpr (plain) proc(c);
                else             if (proc(c)) return faux;
            }
            if constexpr (!plain) return true;
        }
        template<class P>
        void each(rect const& region, P proc) // core: Exec a proc for each cell of the specified region.
        {
            netxs::onrect(*this, region, proc);
        }
        auto copy(grid& target) const // core: Copy only grid of the canvas to the specified grid bitmap.
        {
            target = canvas;
            return region.size;
        }                                                      
        template<class P>
        void copy(core& target, P proc) const // core: Copy the canvas to the specified target bitmap. The target bitmap must be the same size.
        {
            netxs::oncopy(target, *this, proc);
            //todo should we copy all members?
            //target.marker = marker;
            //flow::cursor
        }
        template<class P>
        void fill(core const& block, P fuse) // core: Fill canvas by the specified face using its coordinates.
        {
            netxs::onbody(*this, block, fuse);
        }
        template<class P>
        void plot(core const& block, P fuse) // core: Fill view by the specified face using its coordinates.
        {
            auto joint = view().clip(block.area());
            if (joint)
            {
                auto place = joint.coor - block.coor();
                netxs::inbody<faux>(*this, block, joint, place, fuse);
            }
        }
        template<class P>
        void fill(ui::rect block, P fuse) // core: Process the specified region by the specified proc.
        {
            block.normalize_itself();
            block.coor += region.coor;
            netxs::onrect(*this, block, fuse);
        }
        template<class P>
        void fill(P fuse) // core: Fill the client area using lambda.
        {
            fill(view(), fuse);
        }
        void grad(rgba const& c1, rgba const& c2) // core: Fill the specified region with the linear gradient.
        {
            auto mx = (float)region.size.x;
            auto my = (float)region.size.y;
            auto len = std::sqrt(mx * mx + my * my * 4);

            auto dr = (c2.chan.r - c1.chan.r) / len;
            auto dg = (c2.chan.g - c1.chan.g) / len;
            auto db = (c2.chan.b - c1.chan.b) / len;
            auto da = (c2.chan.a - c1.chan.a) / len;

            si32 x = 0, y = 0, yy = 0;
            auto allfx = [&](cell& c) {
                auto dt = std::sqrt(x * x + yy);
                auto& chan = c.bgc().chan;
                chan.r = (uint8_t)((float)c1.chan.r + dr * dt);
                chan.g = (uint8_t)((float)c1.chan.g + dg * dt);
                chan.b = (uint8_t)((float)c1.chan.b + db * dt);
                chan.a = (uint8_t)((float)c1.chan.a + da * dt);
                ++x;
            };
            auto eolfx = [&]() {
                x = 0;
                ++y;
                yy = y * y * 4;
            };
            netxs::onrect(*this, region, allfx, eolfx);
        }
        void swap(core& target) { canvas.swap(target.canvas); } // core: Unconditionally swap canvases.
        auto swap(grid& target)                                 // core: Move the canvas to the specified array and return the current layout size.
        {
            if (auto size = canvas.size())
            {
                if (target.size() == size) canvas.swap(target);
                else                       target = canvas;
            }
            return region.size;
        }
        template<bool USESGR = true, bool INITIAL = true, bool FINALISE = true>
        auto meta(rect region, cell& state) // core: Ansify/textify content of specified region.
        {
            ansi::esc yield;
            auto badfx = [&](auto& state, auto& frame)
            {
                frame.add(utf::REPLACEMENT_CHARACTER_UTF8_VIEW);
                state.set_gc();
                state.wdt(1);
            };
            auto side_badfx = [&](auto& state, auto& frame) // Restoring the halves on the side
            {
                frame.add(state.txt());
                state.set_gc();
                state.wdt(1);
            };
            auto allfx = [&](cell& c)
            {
                auto width = c.wdt();
                if (width < 2) // Narrow character
                {
                    if (state.wdt() == 2) badfx(state, yield); // Left part alone

                    c.scan<svga::truecolor, USESGR>(state, yield);
                }
                else
                {
                    if (width == 2) // Left part
                    {
                        if (state.wdt() == 2) badfx(state, yield);  // Left part alone

                        c.scan_attr<svga::truecolor, USESGR>(state, yield);
                        state.set_gc(c); // Save char from c for the next iteration
                    }
                    else if (width == 3) // Right part
                    {
                        if (state.wdt() == 2)
                        {
                            if (state.scan<svga::truecolor, USESGR>(c, state, yield)) state.set_gc(); // Cleanup used t
                            else
                            {
                                badfx(state, yield); // Left part alone
                                c.scan_attr<svga::truecolor, USESGR>(state, yield);
                                badfx(state, yield); // Right part alone
                            }
                        }
                        else
                        {
                            c.scan_attr<svga::truecolor, USESGR>(state, yield);
                            if (state.wdt() == 0)
                            {
                                side_badfx(state, yield); // Right part alone at the left side
                            }
                            else
                            {
                                badfx(state, yield); // Right part alone
                            }
                        }
                    }
                }
            };
            auto eolfx = [&]()
            {
                if (state.wdt() == 2) side_badfx(state, yield);  // Left part alone at the right side
                state.set_gc();
                yield.eol();
            };

            if (region)
            {
                if constexpr (USESGR && INITIAL) yield.nil();
                netxs::onrect(*this, region, allfx, eolfx);
                if constexpr (FINALISE)
                {
                    yield.pop_back(); // Pop last eol (lf).
                    if constexpr (USESGR) yield.nil();
                }
            }
            return static_cast<utf::text>(yield);
        }
        template<bool USESGR = true, bool INITIAL = true, bool FINALISE = true>
        auto meta(rect region) // core: Ansify/textify content of specified region.
        {
            cell state;
            return meta<USESGR, INITIAL, FINALISE>(region, state);
        }
        template<bool USESGR = true, bool INITIAL = true, bool FINALISE = true>
        auto meta(cell& state) // core: Ansify/textify all content.
        {
            auto region = rect{-dot_mx / 2, dot_mx };
            return meta<USESGR, INITIAL, FINALISE>(region, state);
        }
        template<feed DIRECTION>
        auto word(twod coord) // core: Detect a word bound.
        {
            if (!region) return 0;
            static constexpr auto rev = DIRECTION == feed::fwd ? faux : true;

            //todo unify
            auto is_empty = [&](auto txt)
            {
                return txt.empty() || txt.front() == whitespace;
            };
            auto empty = [&](auto txt)
            {
                return is_empty(txt);
            };
            auto alpha = [&](auto txt)
            {
                //todo revise (https://unicode.org/reports/tr29/#Word_Boundaries)
                auto c = utf::letter(txt).attr.cdpoint;
                return  c >= '0' && c <= '9' //30-39: '0'-'9'
                     || c >= '@' && c <= 'Z' //40-5A: '@','A'-'Z'
                     || c >= 'a' && c <= 'z' //5F,61-7A: '_','a'-'z'
                     || c == '_'             //60:    '`'
                     || c == 0xA0            //A0  NO-BREAK SPACE (NBSP)
                     || c >= 0xC0                // C0-10FFFF: "À" - ...
                     && c < 0x2000 || c > 0x206F // General Punctuation
                     && c < 0x2200 || c > 0x23FF // Mathematical Operators
                     && c < 0x2500 || c > 0x25FF // Box Drawing
                     && c < 0x2E00 || c > 0x2E7F // Supplemental Punctuation
                     && c < 0x3000 || c > 0x303F // CJK Symbols and Punctuation
                     && c != 0x30FB              // U+30FB ( ・ ) KATAKANA MIDDLE DOT
                     && c < 0xFE50 || c > 0xFE6F // FE50  FE6F Small Form Variants
                     && c < 0xFF00 || c > 0xFF0F // Halfwidth and Fullwidth Forms
                     && c < 0xFF1A || c > 0xFF1F // 
                     && c < 0xFF3B || c > 0xFF40 // 
                     && c < 0xFF5B || c > 0xFF65 // 
            ;};
            auto is_email = [&](auto txt)
            {
                return !txt.empty() && txt.front() == '@';
            };
            auto email = [&](auto txt)
            {
                return !txt.empty() && (alpha(txt) || txt.front() == '.');
            };
            auto is_digit = [&](auto txt)
            {
                auto c = utf::letter(txt).attr.cdpoint;
                return c >= '0' && c <= '9'
                    || c >= 0xFF10 && c <= 0xFF19 // U+FF10 (０) FULLWIDTH DIGIT ZERO - U+FF19 (９) FULLWIDTH DIGIT NINE
                    || c == '.';
            };
            auto digit = [&](auto txt)
            {
                auto c = utf::letter(txt).attr.cdpoint;
                return c == '.'
                    || c >= 'a' && c <= 'f'
                    || c >= 'A' && c <= 'F'
                    || c >= '0' && c <= '9'
                    || c >= 0xFF10 && c <= 0xFF19; // U+FF10 (０) FULLWIDTH DIGIT ZERO - U+FF19 (９) FULLWIDTH DIGIT NINE
            };
            auto func = [&](auto check)
            {
                coord.x += rev ? 1 : 0;
                auto count = decltype(coord.x){};
                auto width = (rev ? 0 : region.size.x) - coord.x;
                auto field = rect{ coord + region.coor, { width, 1 }}.normalize();
                auto allfx = [&](auto& c)
                {
                    auto txt = c.txt();
                    if (!check(txt)) return true;
                    count++;
                    return faux;
                };
                netxs::onrect<rev>(*this, field, allfx);
                if (count) count--;
                coord.x -= rev ? count + 1 : -count;
            };

            coord = std::clamp(coord, dot_00, region.size - dot_11);
            auto test = data(coord)->txt();
            is_digit(test) ? func(digit) :
            is_email(test) ? func(email) :
            is_empty(test) ? func(empty) :
                             func(alpha);
            return coord.x;
        }
        template<class P>
        void cage(ui::rect const& area, twod const& border_width, P fuse) // core: Draw the cage around specified area.
        {
            auto temp = area;
            temp.size.y = border_width.y; // Top
            fill(temp, fuse);
            temp.coor.y += area.size.y - border_width.y; // Bottom
            fill(temp, fuse);
            temp.size.x = border_width.x; // Left
            temp.size.y = area.size.y - border_width.y * 2;
            temp.coor.y = area.coor.y + border_width.y;
            fill(temp, fuse);
            temp.coor.x += area.size.x - border_width.x; // Right
            fill(temp, fuse);
        }
        template<class P>
        void cage(ui::rect const& area, dent const& border, P fuse) // core: Draw the cage around specified area.
        {
            auto temp = area;
            temp.size.y = border.head.step; // Top
            fill(temp, fuse);
            temp.coor.y += area.size.y - border.foot.step; // Bottom
            temp.size.y = border.foot.step;
            fill(temp, fuse);
            temp.size.x = border.west.step; // Left
            temp.size.y = area.size.y - border.head.step - border.foot.step;
            temp.coor.y = area.coor.y + border.head.step;
            fill(temp, fuse);
            temp.coor.x += area.size.x - border.east.step; // Right
            temp.size.x = border.east.step;
            fill(temp, fuse);
        }
        template<class TEXT, class P = noop>
        void text(twod const& pos, TEXT const& txt, bool rtl = faux, P print = P()) // core: Put the specified text substring to the specified coordinates on the canvas.
        {
            rtl ? txt.template output<true>(*this, pos, print)
                : txt.template output<faux>(*this, pos, print);
        }
        template<class SI32>
        auto find(core const& what, SI32&& from, feed dir = feed::fwd) const // core: Find the substring and place its offset in &from.
        {
            assert(     canvas.size() <= std::numeric_limits<si32>::max());
            assert(what.canvas.size() <= std::numeric_limits<si32>::max());
            auto full = static_cast<si32>(     canvas.size());
            auto size = static_cast<si32>(what.canvas.size());
            auto rest = full - from;
            auto look = [&](auto canvas_begin, auto canvas_end, auto what_begin)
            {
                if (!size || size > rest) return faux;

                size--;
                auto head = canvas_begin;
                auto tail = canvas_end - size;
                auto iter = head + from;
                auto base = what_begin;
                auto dest = base;
                auto&test =*base;
                while (iter != tail)
                {
                    if (test.same_txt(*iter++))
                    {
                        auto init = iter;
                        auto stop = iter + size;
                        while (init != stop && init->same_txt(*++dest))
                        {
                            ++init;
                        }

                        if (init == stop)
                        {
                            from = static_cast<si32>(std::distance(head, iter)) - 1;
                            return true;
                        }
                        else dest = base;
                    }
                }
                return faux;
            };

            if (dir == feed::fwd)
            {
                if (look(canvas.begin(), canvas.end(), what.canvas.begin()))
                {
                    return true;
                }
            }
            else
            {
                std::swap(rest, from); // Reverse.
                if (look(canvas.rbegin(), canvas.rend(), what.canvas.rbegin()))
                {
                    from = full - from - 1; // Restore forward representation.
                    return true;
                }
            }
            return faux;
        }
        auto toxy(si32 offset) const // core: Convert offset to coor.
        {
            assert(canvas.size() <= std::numeric_limits<si32>::max());
            auto maxs = static_cast<si32>(canvas.size());
            if (!maxs) return dot_00;
            offset = std::clamp(offset, 0, maxs - 1);
            return twod{ offset % region.size.x,
                         offset / region.size.x };
        }
        auto line(si32 from, si32 upto) const // core: Get stripe.
        {
            if (from > upto) std::swap(from, upto);
            assert(canvas.size() <= std::numeric_limits<si32>::max());
            auto maxs = static_cast<si32>(canvas.size());
            from = std::clamp(from, 0, maxs ? maxs - 1 : 0);
            upto = std::clamp(upto, 0, maxs);
            auto size = upto - from;
            return core{ span{ canvas.data() + from, static_cast<size_t>(size) }, { size, 1 }};
        }
        auto line(twod p1, twod p2) const // core: Get stripe.
        {
            if (p1.y > p2.y || (p1.y == p2.y && p1.x > p2.x)) std::swap(p1, p2);
            auto from = p1.x + p1.y * region.size.x;
            auto upto = p2.x + p2.y * region.size.x + 1;
            return line(from, upto);
        }
        void operator += (core const& src) // core: Append specified canvas.
        {
            //todo inbody::RTL
            auto a_size = size();
            auto b_size = src.size();
            auto new_sz = twod{ a_size.x + b_size.x, std::max(a_size.y, b_size.y) };
            core block{ region.coor, new_sz, marker };

            auto region = rect{ twod{ 0, new_sz.y - a_size.y }, a_size };
            netxs::inbody<faux>(block, *this, region, dot_00, cell::shaders::full);
            region.coor.x += a_size.x;
            region.coor.y += new_sz.y - a_size.y;
            region.size = b_size;
            netxs::inbody<faux>(block, src, region, dot_00, cell::shaders::full);

            swap(block);
            digest++;
        }
    };

    // richtext: The text feeder.
    class flow
        : protected ansi::runtime
    {
        rect textline{ }; // flow: Textline placeholder.
        si32 textsize{ }; // flow: Full textline length (1D).
        side boundary{ }; // flow: Affected area by the text output.
        si32 curpoint{ }; // flow: Current substring start position.
        si32 caret_mx{ }; // flow: Maximum x-coor value on the visible area.
        twod caretpos{ }; // flow: Current virtual (w/o style applied) caret position.
        twod caretsav{ }; // flow: Caret pos saver.
        rect viewrect{ }; // flow: Client area inside page margins.
        rect pagerect{ }; // flow: Client full area. Used as a nested areas (coords++) accumulator.
        rect pagecopy{ }; // flow: Client full area saver.
        deco selfcopy{ }; // flow: Flow state storage.
        deco runstyle{ }; // flow: Flow state.
        si32 highness{1}; // flow: Last processed line height.

        si32 const& size_x;
        si32 const& size_y;

        using hndl = void (*)(flow&, si32);

        // flow: Command list.
        static void exec_dx(flow& f, si32 a) { f.dx(a); }
        static void exec_dy(flow& f, si32 a) { f.dy(a); }
        static void exec_ax(flow& f, si32 a) { f.ax(a); }
        static void exec_ay(flow& f, si32 a) { f.ay(a); }
        static void exec_ox(flow& f, si32 a) { f.ox(a); }
        static void exec_oy(flow& f, si32 a) { f.oy(a); }
        static void exec_px(flow& f, si32 a) { f.px(a); }
        static void exec_py(flow& f, si32 a) { f.py(a); }
        static void exec_tb(flow& f, si32 a) { f.tb(a); }
        static void exec_nl(flow& f, si32 a) { f.nl(a); }
        static void exec_sc(flow& f, si32 a) { f.sc( ); }
        static void exec_rc(flow& f, si32 a) { f.rc( ); }
        static void exec_zz(flow& f, si32 a) { f.zz( ); }

        // flow: Draw commands (customizable).
        template<ansi::fn CMD>
        static void exec_dc(flow& f, si32 a) { if (f.custom) f.custom(CMD, a); }
        //static void exec_dc(flow& f, si32 a) { f.custom(CMD, a); }

        // flow: Abstract handler
        //       ansi::fn::ed
        //       ansi::fn::el
        //virtual void custom(ansi::fn cmd, si32 arg) = 0;

        constexpr static std::array<hndl, ansi::fn_count> exec =
        {   // Order does matter, see definition of ansi::fn.
            exec_dx, // horizontal delta
            exec_dy, // vertical delta
            exec_ax, // x absolute
            exec_ay, // y absolute
            exec_ox, // old format x absolute (1-based)
            exec_oy, // old format y absolute (1-based)
            exec_px, // x percent
            exec_py, // y percent
            exec_tb, // horizontal tab
            exec_nl, // next line and reset x to west (carriage return)

            exec_sc, // save cursor position
            exec_rc, // load cursor position
            exec_zz, // all params reset to zero

            exec_dc<ansi::fn::ed>, // CSI Ps J  Erase in Display (ED)
            exec_dc<ansi::fn::el>, // CSI Ps K  Erase in Line    (EL)
        };

        // flow: Main cursor forwarding proc.
        template<bool SPLIT, bool WRAP, bool RtoL, bool ReLF, class T, class P>
        void output(T const& block, P print)
        {
            textline.coor = caretpos;

            rect printout;
            si32 outwidth;
            if constexpr (WRAP)
            {
                printout = textline.trunc(viewrect.size);
                outwidth = printout.coor.x + printout.size.x - textline.coor.x;

                if constexpr (!SPLIT)
                if (printout.size.x > 1)
                {
                    auto p = curpoint + printout.size.x - 1;
                    if (block.at(p).wdt() == 2)
                    {
                        --printout.size.x;
                    }
                }
            }
            else
            {
                printout = textline;
                outwidth = textline.size.x;
            }

            //flow::up();
            flow::dx(outwidth);
            //flow::up();

            auto startpos = curpoint;
            curpoint += std::max(printout.size.x, 1);
            textline.size.x = textsize - curpoint;

            if (RtoL) printout.coor.x = viewrect.size.x - (printout.coor.x + printout.size.x);
            if (ReLF) printout.coor.y = viewrect.size.y - (printout.coor.y + printout.size.y);
            else      printout.coor.y = textline.coor.y; //do not truncate y-axis?
            //todo revise: It is actually only for the coor.y that is negative.

            printout.coor += viewrect.coor;
            boundary |= printout;

            if constexpr (!std::is_same_v<P, noop>)
            {
                //todo margins don't apply to unwrapped text
                //auto imprint = WRAP ? printout
                //                    : viewrect.clip(printout);
                if (printout)
                {
                    auto& coord = printout.coor;
                    auto& width = printout.size.x;
                    auto& start = straight ? startpos
                                           : textline.size.x;
                    print(coord, block.substr(start, width), runtime::isr_to_l);
                }
            }
            highness = textline.size.y;
        }

        auto middle() { return (viewrect.size.x >> 1) - (textline.size.x >> 1); }
        void autocr() { if (caretpos.x >= caret_mx) flow::nl(highness); }

        template<bool SPLIT, bool RtoL, bool ReLF, class T, class P>
        void centred(T const& block, P print)
        {
            while (textline.size.x > 0)
            {
                autocr();
                auto axis = textline.size.x >= caret_mx ? 0
                                                        : middle();
                flow::ax(axis);
                output<SPLIT, true, RtoL, ReLF>(block, print);
            }
        }
        template<bool SPLIT, bool RtoL, bool ReLF, class T, class P>
        void wrapped(T const& block, P print)
        {
            while (textline.size.x > 0)
            {
                autocr();
                output<SPLIT, true, RtoL, ReLF>(block, print);
            }
        }
        template<bool SPLIT, bool RtoL, bool ReLF, class T, class P>
        void trimmed(T const& block, P print)
        {
            if (textline.size.x > 0)
            {
                if (centered) flow::ax(middle());
                output<SPLIT, faux, RtoL, ReLF>(block, print);
            }
        }
        template<bool SPLIT, bool RtoL, bool ReLF, class T, class P>
        void proceed(T const& block, P print)
        {
            if (iswrapln) if (centered) centred<SPLIT, RtoL, ReLF>(block, print);
                          else          wrapped<SPLIT, RtoL, ReLF>(block, print);
            else                        trimmed<SPLIT, RtoL, ReLF>(block, print);
        }

        std::function<void(ansi::fn cmd, si32 arg)> custom; // flow: Draw commands (customizable).

    public:
        flow(si32 const& size_x, si32 const& size_y)
            : size_x { size_x },
              size_y { size_y }
        { }
        flow(twod const& size )
            : flow { size.x, size.y }
        { }
        flow()
            : flow { pagerect.size }
        { }

        void vsize(si32 height) { pagerect.size.y = height;  } // flow: Set client full height.
        void  size(twod const& size) { pagerect.size = size; } // flow: Set client full size.
        void  full(rect const& area) { pagerect = area;      } // flow: Set client full rect.
        auto& full() const           { return pagerect;      } // flow: Get client full rect reference.
        auto& minmax() const         { return boundary;      } // flow: Return the output range.
        void  minmax(twod const& p)  { boundary |= p;        } // flow: Register twod.
        void  minmax(rect const& r)  { boundary |= r;        } // flow: Register rect.

        // flow: Split specified textblock on the substrings
        //       and place it to the form by the specified proc.
        template<bool SPLIT, class T, class P = noop>
        void compose(T const& block, P print = P())
        {
            combine(runstyle, block.style);

            auto block_size = block.size();
            textsize = getlen(block_size);
            if (textsize)
            {
                textline = getvol(block_size);
                curpoint = 0;
                viewrect = textpads.area(size_x, size_y);
                viewrect.coor += pagerect.coor;
                caret_mx = viewrect.size.x;

                // Move cursor down if next line is lower than previous.
                if (highness > textline.size.y)
                //if (highness != textline.size.y)
                {
                    dy(highness - textline.size.y);
                    highness = textline.size.y;
                }

                if (arighted)
                {
                    if (isrlfeed) proceed<SPLIT, true, true>(block, print);
                    else          proceed<SPLIT, true, faux>(block, print);
                }
                else
                {
                    if (isrlfeed) proceed<SPLIT, faux, true>(block, print);
                    else          proceed<SPLIT, faux, faux>(block, print);
                }
            }
        }
        // flow: Execute specified locus instruction list.
        auto forward(writ const& cmd)
        {
            auto& inst = *this;
            //flow::up();
            for (auto [cmd, arg] : cmd)
            {
                flow::exec[cmd](inst, arg);
            }
            return flow::up();
        }
        template<bool SPLIT = true, class T>
        void go(T const& block)
        {
            compose<SPLIT>(block);
        }
        template<bool SPLIT = true, class T, class P = noop>
        void go(T const& block, core& canvas, P printfx = P())
        {
            compose<SPLIT>(block, [&](auto const& coord, auto const& subblock, auto isr_to_l)
                                  {
                                      canvas.text(coord, subblock, isr_to_l, printfx);
                                  });
        }
        template<bool USE_LOCUS = true, class T, class P = noop>
        auto print(T const& block, core& canvas, P printfx = P())
        {
            using type = std::invoke_result_t<decltype(&flow::cp), flow>;
            type coor;

            if constexpr (USE_LOCUS) coor = forward(block);
            else                     coor = flow::cp();

            go<faux>(block, canvas, printfx);
            return coor;
        }
        template<bool USE_LOCUS = true, class T>
        auto print(T const& block)
        {
            using type = std::invoke_result_t<decltype(&flow::cp), flow>;
            type coor;

            if constexpr (USE_LOCUS) coor = forward(block);
            else                     coor = flow::cp();

            go<faux>(block);
            return coor;
        }

        void ax	(si32 x)        { caretpos.x  = x;                 }
        void ay	(si32 y)        { caretpos.y  = y;                 }
        void ac	(twod const& c) { ax(c.x); ay(c.y);                }
        void ox	(si32 x)        { caretpos.x  = x - 1;             }
        void oy	(si32 y)        { caretpos.y  = y - 1;             }
        void oc	(twod const& c) { ox(c.x); oy(c.y);                }
        void dx	(si32 n)        { caretpos.x += n;                 }
        void dy	(si32 n)        { caretpos.y += n;                 }
        void nl	(si32 n)        { ax(0); dy(n);                    }
        void px	(si32 x)        { ax(textpads.h_ratio(x, size_x)); }
        void py	(si32 y)        { ay(textpads.v_ratio(y, size_y)); }
        void pc	(twod const& c) { px(c.x); py(c.y);                }
        void tb	(si32 n)
        {
            if (n)
            {
                dx(tabwidth - caretpos.x % tabwidth);
                if (n > 0 ? --n : ++n) dx(tabwidth * n);
            }
        }
        twod cp () const // flow: Return absolute cursor position.
        {
            twod coor{ caretpos };
            if (arighted) coor.x = textpads.width (size_x) - 1 - coor.x;
            if (isrlfeed) coor.y = textpads.height(size_y) - 1 - coor.y;
            coor += textpads.corner();
            return coor;
        }
        twod up () // flow: Register cursor position.
        {
            auto cp = flow::cp();
            boundary |= cp; /* |= cursor*/;
            return cp;
        }
        void zz (twod const& offset = dot_00)
        {
            runstyle.glb();
            caretpos = dot_00;
            pagerect.coor = offset;
        }
        void sc () // flow: Save current state.
        {
            selfcopy = runstyle;
            caretsav = caretpos;
            pagecopy.coor = pagerect.coor;
        }
        void rc () // flow: Restore state.
        {
            runstyle = selfcopy;
            caretpos = caretsav;
            pagerect.coor = pagecopy.coor;
        }
        auto get_state()
        {
            return std::tuple<deco,twod,twod>{ runstyle, caretpos, pagerect.coor };
        }
        template<class S>
        auto set_state(S const& state)
        {
            runstyle      = std::get<0>(state);
            caretpos      = std::get<1>(state);
            pagerect.coor = std::get<2>(state);
        }
        void reset(twod const& offset = dot_00) // flow: Reset flow state.
        {
            flow::zz(offset);
            flow::sc();
            boundary = caretpos;
        }
        void reset(flow const& canvas) // flow: Reset flow state.
        {
            reset(canvas.pagerect.coor);
        }
        void set_style(deco const& new_style)
        {
            runstyle = new_style;
        }
        auto get_style()
        {
            return runstyle;
        }
    };

    // richtext: The shadow of the para.
    class shot
    {
        core const& basis;
        si32        start;
        si32        width;

    public:
        constexpr
        shot(shot const&) = default;

        constexpr
        shot(core const& basis, si32 begin, si32 piece)
            : basis{ basis },
              start{ std::max(0, begin) },
              width{ std::min(std::max(0, piece), basis.size().x - start) }
        {
            if (basis.size().x <= start)
            {
                start = 0;
                width = 0;
            }
        }

        constexpr
        shot(core const& basis)
            : basis{ basis },
              start{       },
              width{ basis.size().x }
        { }

        constexpr
        auto substr(si32 begin, si32 piece = netxs::maxsi32) const
        {
            auto w = basis.size().x;
            auto a = start + std::max(begin, 0);
            return a < w ? shot{ basis, a, std::min(std::max(piece, 0), w - a) }
                         : shot{ basis, 0, 0 };
        }

                  auto& mark  () const { return  basis.mark();         }
                  auto  data  () const { return  basis.data() + start; }
        constexpr auto  size  () const { return  basis.size();         }
        constexpr auto  empty () const { return !width;                }
        constexpr auto  length() const { return  width;                }

        template<bool RtoL, class P = noop>
        auto output(core& canvas, twod const& pos, P print = P()) const  // shot: Print the source content using the specified print proc, which returns the number of characters printed.
        {
            //todo place is wrong if RtoL==true
            //rect place{ pos, { RtoL ? width, basis.size().y } };
            //auto joint = canvas.view().clip(place);
            rect place{ pos, { width, basis.size().y } };
            auto joint = canvas.view().clip(place);
            //auto joint = canvas.area().clip(place);

            if (joint)
            {
                if constexpr (RtoL)
                {
                    place.coor.x += place.size.x - joint.coor.x - joint.size.x;
                    place.coor.y  = joint.coor.y - place.coor.y;
                }
                else
                {
                    place.coor = joint.coor - place.coor;
                }
                place.coor.x += start;

                if constexpr (std::is_same_v<P, noop>) netxs::inbody<RtoL>(canvas, basis, joint, place.coor, cell::shaders::fusefull);
                else                                   netxs::inbody<RtoL>(canvas, basis, joint, place.coor, print);
            }

            return width;
        }
    };

    // richtext: Enriched text line.
    class rich
        : public core
    {
    public:
        using core::core;

        rich(core&& s)
            : core{ std::forward<core>(s) }
        { }

        auto length() const                                     { return size().x;                         }
        auto shadow() const                                     { return shot{ *this };                    }
        auto substr(si32 at, si32 width = netxs::maxsi32) const { return shadow().substr(at, width);       }
        void trimto(si32 max_size)                              { if (length() > max_size) crop(max_size); }
        void reserv(si32 oversize)                              { if (oversize > length()) crop(oversize); }
        auto empty()
        {
            return canvas.empty();
        }
        void shrink(cell const& blank, si32 max_size = 0, si32 min_size = 0)
        {
            assert(min_size <= length());
            auto head = iter();
            auto tail = iend();
            auto stop = head + min_size;
            while (stop != tail-- && *tail == blank) { }
            auto new_size = static_cast<si32>(tail - head + 1);
            if (max_size && max_size < new_size) new_size = max_size;
            if (new_size != length()) crop(new_size);
        }
        template<bool AUTOGROW = faux>
        void splice(si32 at, si32 count, cell const& blank)
        {
            if (count <= 0) return;
            auto len = length();
            if constexpr (AUTOGROW) reserv(at + count);
            else
            {
                if (at >= len) return;
                count = std::min(count, len - at);
            }
            auto ptr = iter();
            auto dst = ptr + at;
            auto end = dst + count;
            while (dst != end) *dst++ = blank;
        }
        void splice(si32 at, shot const& fragment)
        {
            auto len = fragment.length();
            reserv(len + at);
            auto ptr = iter();
            auto dst = ptr + at;
            auto end = dst + len;
            auto src = fragment.data();
            while (dst != end) *dst++ = *src++;
        }
        template<class SRC_IT, class DST_IT>
        static void forward_fill_proc(SRC_IT data, DST_IT dest, DST_IT tail)
        {
            //  + evaluate TAB etc
            //  + bidi
            //  + eliminate non-printable and with cwidth == 0 (\0, \t, \b, etc...)
            //  + while (--wide)
            //    {
            //        /* IT IS UNSAFE IF REALLOCATION OCCURS. BOOK ALWAYS */
            //        lyric.emplace_back(cluster, console::whitespace);
            //    }
            //  + convert front into the screen-like sequence (unfold, remmove zerospace chars)

            --tail; /* tail - 1: half of the wide char */;
            while (dest < tail)
            {
                auto c = *data++;
                auto w = c.wdt();
                if (w == 1)
                {
                    *dest++ = c;
                }
                else if (w == 2)
                {
                    *dest++ = c.wdt(2);
                    *dest++ = c.wdt(3);
                }
                else if (w == 0)
                {
                    //todo implemet controls/commands
                    // winsrv2019's cmd.exe sets title with a zero at the end
                    //*dst++ = cell{ c, whitespace };
                }
                else if (w > 2)
                {
                    // Forbid using super wide characters until terminal emulators support the fragmentation attribute.
                    c.txt(utf::REPLACEMENT_CHARACTER_UTF8_VIEW);
                    do *dest++ = c;
                    while (--w && dest != tail + 1);
                }
            }
            if (dest == tail) // Last cell; tail - 1.
            {
                auto c = *data;
                auto w = c.wdt();
                     if (w == 1) *dest = c;
                else if (w == 2) *dest = c.wdt(3);
                else if (w >  2) *dest = c.txt(utf::REPLACEMENT_CHARACTER_UTF8_VIEW);
            }
        }
        template<class SRC_IT, class DST_IT>
        static void unlimit_fill_proc(SRC_IT data, si32 size, DST_IT dest, DST_IT tail, si32 back)
        {
            //  + evaluate TAB etc
            //  + bidi
            //  + eliminate non-printable and with cwidth == 0 (\0, \t, \b, etc...)
            //  + while (--wide)
            //    {
            //        /* IT IS UNSAFE IF REALLOCATION OCCURS. BOOK ALWAYS */
            //        lyric.emplace_back(cluster, console::whitespace);
            //    }
            //  + convert front into the screen-like sequence (unfold, remmove zerospace chars)

            auto set = [&](auto const& c)
            {
                if (dest == tail) dest -= back;
                *dest++ = c;
                --size;
            };
            while (size > 0)
            {
                auto c = *data++;
                auto w = c.wdt();
                if (w == 1)
                {
                    set(c);
                }
                else if (w == 2)
                {
                    set(c.wdt(2));
                    set(c.wdt(3));
                }
                else if (w == 0)
                {
                    //todo implemet controls/commands
                    // winsrv2019's cmd.exe sets title with a zero at the end
                    //*dst++ = cell{ c, whitespace };
                }
                else if (w > 2)
                {
                    // Forbid using super wide characters until terminal emulators support the fragmentation attribute.
                    c.txt(utf::REPLACEMENT_CHARACTER_UTF8_VIEW);
                    do set(c);
                    while (--w && size > 0);
                }
            }
        }
        template<class SRC_IT, class DST_IT>
        static void reverse_fill_proc(SRC_IT data, DST_IT dest, DST_IT tail)
        {
            //  + evaluate TAB etc
            //  + bidi
            //  + eliminate non-printable and with cwidth == 0 (\0, \t, \b, etc...)
            //  + while (--wide)
            //    {
            //        /* IT IS UNSAFE IF REALLOCATION OCCURS. BOOK ALWAYS */
            //        lyric.emplace_back(cluster, console::whitespace);
            //    }
            //  + convert front into the screen-like sequence (unfold, remmove zerospace chars)

            ++tail; /* tail + 1: half of the wide char */;
            while (dest > tail)
            {
                auto c = *--data;
                auto w = c.wdt();
                if (w == 1)
                {
                    *--dest = c;
                }
                else if (w == 2)
                {
                    *--dest = c.wdt(3);
                    *--dest = c.wdt(2);
                }
                else if (w == 0)
                {
                    //todo implemet controls/commands
                    // winsrv2019's cmd.exe sets title with a zero at the end
                    //*dst++ = cell{ c, whitespace };
                }
                else if (w > 2)
                {
                    // Forbid using super wide characters until terminal emulators support the fragmentation attribute.
                    c.txt(utf::REPLACEMENT_CHARACTER_UTF8_VIEW);
                    do *--dest = c;
                    while (--w && dest != tail - 1);
                }
            }
            if (dest == tail) // Last cell; tail + 1.
            {
                auto c = *--data;
                auto w = c.wdt();
                     if (w == 1) *--dest = c;
                else if (w == 2) *--dest = c.wdt(3);
                else if (w >  2) *--dest = c.txt(utf::REPLACEMENT_CHARACTER_UTF8_VIEW);
            }
        }
        void splice(si32 at, si32 count, grid const& proto)
        {
            if (count <= 0) return;
            reserv(at + count);
            auto end = iter() + at;
            auto dst = end + count;
            auto src = proto.end();
            reverse_fill_proc(src, dst, end);
        }
        void splice(twod at, si32 count, grid const& proto)
        {
            if (count <= 0) return;
            auto end = iter() + at.x + at.y * size().x;
            auto dst = end + count;
            auto src = proto.end();
            reverse_fill_proc(src, dst, end);
        }
        // rich: Scroll by gap the 2D-block of lines between top and end (exclusive); down: gap > 0; up: gap < 0.
        void scroll(si32 top, si32 end, si32 gap, cell const& clr)
        {
            auto data = core::data();
            auto size = core::size();
            auto step = size.x;
            assert(top >= 0 && top < end && end <= size.y);
            assert(gap != 0);
            if (gap > 0)
            {
                auto src = top * size.x + data;
                auto cut = end - gap;
                if (cut > top)
                {
                    step *= gap;
                    auto head = data + size.x * cut;
                    auto dest = head + step;
                    auto tail = src;
                    while (head != tail) *--dest = *--head;
                }
                else step *= end - top;

                auto dst = src + step;
                while (dst != src) *src++ = clr;
            }
            else
            {
                auto src = end * size.x + data;
                auto cut = top - gap;
                if (cut < end)
                {
                    step *= gap;
                    auto head = data + size.x * cut;
                    auto dest = head + step;
                    auto tail = src;
                    while (head != tail) *dest++ = *head++;
                }
                else step *= top - end;

                auto dst = src + step;
                while (dst != src) *--src = clr;
            }
        }
        // rich: (current segment) Insert n blanks at the specified position. Autogrow within segment only.
        void insert(si32 at, si32 count, cell const& blank, si32 margin)
        {
            if (count <= 0) return;
            auto len = length();
            auto pos = at % margin;
            auto vol = std::min(count, margin - pos);
            auto max = std::min(len + vol, at + margin - pos);
            reserv(max);
            auto ptr = iter();
            auto dst = ptr + max;
            auto src = dst - vol;
            auto end = ptr + at;
            while (src != end) *--dst = *--src;
            while (dst != end) *--dst = blank;
        }
        // rich: (whole line) Insert n blanks at the specified position. Autogrow.
        void insert_full(si32 at, si32 count, cell const& blank)
        {
            if (count <= 0) return;
            auto len = length();
            crop(std::max(at, len) + count);
            auto ptr = iter();
            auto pos = ptr + at;
            auto end = pos + count;
            auto src = ptr + len;
            if (at < len)
            {
                auto dst = src + count;
                while (dst != end) *--dst = *--src;
                src = pos;
            }
            while (src != end) *src++ = blank;
        }
        // rich: (current segment) Delete n chars and add blanks at the right margin.
        void cutoff(si32 at, si32 count, cell const& blank, si32 margin)
        {
            if (count <= 0) return;
            auto len = length();
            if (at < len)
            {
                auto pos = at % margin;
                auto rem = std::min(margin - pos, len - at);
                auto vol = std::min(count, rem);
                auto dst = iter() + at;
                auto end = dst + rem;
                auto src = dst + vol;
                while (src != end) *dst++ = *src++;
                while (dst != end) *dst++ = blank;
            }
        }
        // rich: (whole line) Delete n chars and add blanks at the right margin.
        void cutoff_full(si32 at, si32 count, cell const& blank, si32 margin)
        {
            if (count <= 0) return;
            auto len = length();
            if (at < len)
            {
                margin -= at % margin;
                count = std::min(count, margin);
                if (count >= len - at)
                {
                    auto ptr = iter();
                    auto dst = ptr + at;
                    auto end = ptr + len;
                    while (dst != end) *dst++ = blank;
                }
                else
                {
                    reserv(margin + at);
                    auto ptr = iter();
                    auto dst = ptr + at;
                    auto src = dst + count;
                    auto end = dst - count + margin;
                    while (dst != end) *dst++ = *src++;
                    end += count; //end = ptr + right_margin;
                    while (dst != end) *dst++ = blank;
                }
            }
        }
        // rich: Put n blanks on top of the chars and cut them off with the right edge.
        void splice(twod const& at, si32 count, cell const& blank)
        {
            if (count <= 0) return;
            auto len = size();
            auto vol = std::min(count, len.x - at.x);
            assert(at.x + at.y * len.x + vol <= len.y * len.x);
            auto ptr = iter();
            auto dst = ptr + at.x + at.y * len.x;
            auto end = dst + vol;
            while (dst != end) *dst++ = blank;
        }
        // rich: Insert n blanks by shifting chars to the right. Same as delete(twod), but shifts from left to right.
        void insert(twod const& at, si32 count, cell const& blank)
        {
            if (count <= 0) return;
            auto len = size();
            auto vol = std::min(count, len.x - at.x);
            assert(at.x + at.y * len.x + vol <= len.y * len.x);
            auto ptr = iter();
            auto pos = ptr + at.y * len.x;
            auto dst = pos + len.x;
            auto end = pos + at.x;
            auto src = dst - vol;
            while (src != end) *--dst = *--src;
            while (dst != end) *--dst = blank;
        }
        // rich: Delete n chars and add blanks at the right. Same as insert(twod), but shifts from right to left.
        void cutoff(twod const& at, si32 count, cell const& blank)
        {
            if (count <= 0) return;
            auto len = size();
            auto vol = std::min(count, len.x - at.x);
            assert(at.x + at.y * len.x + vol <= len.y * len.x);
            auto ptr = iter();
            auto pos = ptr + at.y * len.x;
            auto dst = pos + at.x;
            auto end = pos + len.x;
            auto src = dst + vol;
            while (src != end) *dst++ = *src++;
            while (dst != end) *dst++ = blank;
        }
        // rich: Clear from the specified coor to the bottom.
        void del_below(twod const& pos, cell const& blank)
        {
            auto len = size();
            auto ptr = iter();
            auto dst = ptr + std::min<si32>(pos.x + pos.y * len.x,
                                                    len.y * len.x);
            auto end = iend();
            while (dst != end) *dst++ = blank;
        }
        // rich: Clear from the top to the specified coor.
        void del_above(twod const& pos, cell const& blank)
        {
            auto len = size();
            auto dst = iter();
            auto end = dst + std::min<si32>(pos.x + pos.y * len.x,
                                                    len.y * len.x);
            while (dst != end) *dst++ = blank;
        }

        //todo unify
        auto& at(si32 p) const
        {
            assert(p >= 0);
            return *(core::data() + p);
        }
    };

    // richtext: Enriched text paragraph.
    class para
        : public ansi::parser
    {
        using corx = sptr<rich>;

    public:
        si32 caret = 0; // para: Cursor position inside lyric.
        ui32 index = 0;
        writ locus;
        corx lyric = std::make_shared<rich>();

        using parser::parser;
        para()                         = default;
        para(para&&)                   = default;
        para(para const&)              = default;
        para& operator = (para&&)      = default;
        para& operator = (para const&) = default;
        para(ui32 newid, deco const& style = {})
            : parser{ style },
              index { newid }
        { }

        para              (view utf8) {              ansi::parse(utf8, this);               }
        auto& operator  = (view utf8) { wipe(brush); ansi::parse(utf8, this); return *this; }
        auto& operator += (view utf8) {              ansi::parse(utf8, this); return *this; }

        operator writ const& () const { return locus; }

        void decouple() { lyric = std::make_shared<rich>(*lyric); } // para: Make canvas isolated copy.
        auto& content() const { return *lyric; } // para: Return paragraph content.
        shot   shadow() const { return *lyric; } // para: Return paragraph shadow.
        shot   substr(si32 start, si32 width) const // para: Return paragraph substring shadow.
        {
            return shadow().substr(start, width);
        }
        bool   bare() const { return locus.bare();    } // para: Does the paragraph have no locator.
        auto length() const { return lyric->size().x; } // para: Return printable length.
        auto   step() const { return count;           } // para: The next caret step.
        auto   size() const { return lyric->size();   } // para: Return 2D volume size.
        auto&  back() const { return brush;           } // para: Return current brush.
        bool   busy() const { return length() || !proto.empty() || brush.busy(); } // para: Is it filled.
        void   ease()   { brush.nil(); lyric->each([&](auto& c) { c.clr(brush); });  } // para: Reset color for all text.
        void   link(id_t id)         { lyric->each([&](auto& c) { c.link(id);   });  } // para: Set object ID for each cell.
        void   wipe(cell c = cell{}) // para: Clear the text and locus, and reset SGR attributes.
        {
            count = 0;
            caret = 0;
            brush.reset(c);
            //todo revise
            //style.rst();
            //proto.clear();
            locus.kill();
            lyric->kill();
        }
        void task(ansi::rule const& cmd) { if (!busy()) locus.push(cmd); } // para: Add locus command. In case of text presence try to change current target otherwise abort content building.
        // para: Convert into the screen-adapted sequence (unfold, remove zerospace chars, etc.).
        void data(si32 count, grid const& proto) override
        {
            lyric->splice(caret, count, proto);
            caret += count;
        }
        void id(ui32 newid) { index = newid; }
        auto id() const     { return index;  }

        auto& set(cell const& c) { brush.set(c); return *this; }

        //todo unify
        auto& at(si32 p) const { return lyric->data(p); } // para: .
    };

    // richtext: Cascade of the identical paragraphs.
    class rope
    {
        using iter = std::list<sptr<para>>::const_iterator;
        iter source;
        iter finish;
        si32 prefix;
        si32 suffix;
        twod volume; // Rope must consist of text lines of the same height.

        rope(iter& source, si32 prefix, iter& finish, si32 suffix, twod const& volume)
            : source{ source },
              prefix{ prefix },
              finish{ finish },
              suffix{ suffix },
              volume{ volume }
              ,style{(**source).style}
        { }

    public:
        deco style;

        rope(iter const& head, iter const& tail, twod const& size)
            : source{ head },
              finish{ tail },
              prefix{ 0    },
              suffix{ 0    },
              volume{ size }
              ,style{(**head).style}
        { }

        operator writ const& () const { return **source; }

        // rope: Return a substring rope the source content.
        //       ! No checking of boundaries !
        rope substr(si32 start, si32 width) const
        {
            auto first = source;
            auto piece = (**first).size().x;

            while (piece <= start)
            {
                start -= piece;
                piece = (**++first).size().x;
            }

            auto end = first;
            piece -= start;
            while (piece < width)
            {
                piece += (**++end).size().x;
            }
            piece -= width;

            return rope{ first, start, end, piece, { width, volume.y } };
        }
        // rope: Print the source content using the specified print proc,
        //       which returns the number of characters printed.
        template<bool RtoL, class P = noop>
        void output(core& canvas, twod locate, P print = P()) const
        {
            auto total = volume.x;

            auto draw = [&](auto& item, auto start, auto width)
            {
                auto line = item.substr(start, width);
                auto size = line.template output<RtoL>(canvas, locate, print);
                locate.x += size;
                return size;
            };

            if constexpr (RtoL)
            {
                auto crop = [](auto piece, auto total, auto& start, auto& width)
                {
                    if (piece > total)
                    {
                        start = piece - total;
                        width = total;
                    }
                    else
                    {
                        start = 0;
                        width = piece;
                    }
                };

                auto refer = finish;
                auto& item = **refer;
                auto piece = item.size().x - suffix;

                si32 start, width, yield;
                crop(piece, total, start, width);
                yield = draw(item, start, width);

                while (total -= yield)
                {
                    auto& item = **--refer;
                    piece = item.size().x;

                    crop(piece, total, start, width);
                    yield = draw(item, start, width);
                }
            }
            else
            {
                auto refer = source;
                auto& item = **refer;
                auto yield = draw(item, prefix, total);

                while (total -= yield)
                {
                    auto& item = **++refer;
                    yield = draw(item, 0, total);
                }
            }
        }
        constexpr
        auto  size  () const { return volume;            } // rope: Return volume of the source content.
        auto  length() const { return volume.x;          } // rope: Return the length of the source content.
        auto  id    () const { return (**source).id();   } // rope: Return paragraph id.
        auto& front () const { return (**source).at(prefix); } // rope: Return first cell.

        //todo unify
        auto& at(si32 p) const // rope: .
        {
            auto shadow = substr(p, 1);
            return shadow.front();
        }
    };

    // richtext: Enriched text page.
    class page
        : public ansi::parser
    {
        using list = std::list<sptr<para>>;
        using iter = list::iterator;
        using pmap = std::map<si32, wptr<para>>;

    public:
        ui32 index = {};              // page: Current paragraph id.
        list batch = { std::make_shared<para>(index) }; // page: Paragraph list.
        iter layer = batch.begin();   // page: Current paragraph.
        pmap parts;                   // page: Paragraph index.

        //todo use ring
        si32 limit = std::numeric_limits<si32>::max(); // page: Paragraphs number limit.
        void shrink() // page: Remove over limit paragraphs.
        {
            auto size = batch.size();
            if (size > limit)
            {
                auto item = static_cast<si32>(std::distance(batch.begin(), layer));
                while (batch.size() > limit)
                {
                    batch.pop_front();
                }
                batch.front()->locus.clear();
                // Update current layer ptr if it gets out.
                if (item < size - limit) layer = batch.begin();
            }
        }
        void maxlen(si32 m) { limit = std::max(1, m); shrink(); } // page: Set the limit of paragraphs.
        auto maxlen() { return limit; } // page: Get the limit of paragraphs.

        using ring = generics::ring<std::vector<para>>;
        struct buff : public ring
        {
            using ring::ring;
            //void undock_base_front(para& p) override { }
            //void undock_base_back (para& p) override { }
        };

        template<class T>
        static void parser_config(T& vt)
        {
            using namespace netxs::ansi;
            vt.intro[ctrl::CR ]              = VT_PROC{ q.pop_if(ctrl::EOL); p->task({ fn::nl,1 }); };
            vt.intro[ctrl::TAB]              = VT_PROC{ p->task({ fn::tb, q.pop_all(ctrl::TAB) }); };
            vt.intro[ctrl::EOL]              = VT_PROC{ p->task({ fn::nl, q.pop_all(ctrl::EOL) }); };
            vt.csier.table[CSI__ED]          = VT_PROC{ p->task({ fn::ed, q(0) }); }; // CSI Ps J
            vt.csier.table[CSI__EL]          = VT_PROC{ p->task({ fn::el, q(0) }); }; // CSI Ps K
            vt.csier.table[CSI_CCC][CCC_NOP] = VT_PROC{ p->fork(); };
            vt.csier.table[CSI_CCC][CCC_IDX] = VT_PROC{ p->fork(q(0)); };
            vt.csier.table[CSI_CCC][CCC_REF] = VT_PROC{ p->bind(q(0)); };
        }
        page              (view utf8) {          ansi::parse(utf8, this);               }
        auto& operator  = (view utf8) { clear(); ansi::parse(utf8, this); return *this; }
        auto& operator += (view utf8) {          ansi::parse(utf8, this); return *this; }

        page ()                         = default;
        page (page&&)                   = default;
        page (page const&)              = default;
        page& operator =  (page const&) = default;
        auto& operator += (page const& p)
        {
            parts.insert(p.parts.begin(), p.parts.end()); // Part id should be unique across pages
            //batch.splice(std::next(layer), p.batch);
            for (auto& a: p.batch)
            {
                batch.push_back(a);
                batch.back()->id(++index);
            }
            shrink();
            layer = std::prev(batch.end());
            return *this;
        }
        // page: Acquire para by id.
        auto& operator [] (si32 id)
        {
            if (netxs::on_key(parts, id))
            {
                if (auto item = parts[id].lock()) return *item;
            }
            fork(id);
            parts.emplace(id, *layer);
            return **layer;
        }
        // page: Clear the list of paragraphs.
        page& clear(bool preserve_state = faux)
        {
            if (!preserve_state) parser::brush.reset();
            parts.clear();
            batch.resize(1);
            layer = batch.begin();
            index = 0;
            auto& item = **layer;
            item.id(index);
            item.wipe(parser::brush);
            return *this;
        }
        // page: Disintegrate the page content into atomic contiguous pieces - ropes.
        //       Call publish(rope{first, last, length}):
        //       a range of [ first,last ] is the uniform consecutive paragraphs set.
        //       Length is the sum of the lengths of the paragraphs.
        template<class F>
        void stream(F publish) const
        {
            twod next;
            auto last = batch.begin();
            auto tail = batch.end();
            while (last != tail)
            {
                auto size = (**last).size();
                auto head = last;
                while (++last != tail
                      && (**last).bare()
                      && size.y == (next = (**last).size()).y)
                {
                    size.x += next.x;
                }
                publish(rope{ head, std::prev(last), size });
            }
        }
        // page: Split the text run.
        template<bool FLUSH = true>
        void fork()
        {
            if constexpr (FLUSH) parser::flush();
            layer = batch.insert(std::next(layer), std::make_shared<para>(parser::style));
            (**layer).id(++index);
            shrink();
        }
        // page: Split the text run and associate the next paragraph with id.
        void fork(si32 id)
        {
            fork();
            parts[id] = *layer;
        }
        void test()
        {
            parser::flush();
            if ((**layer).busy()) fork<faux>();
        }
        // page: Make a shared copy of lyric of existing paragraph.
        void bind(si32 id)
        {
            test();
            auto it = parts.find(id);
            if (it != parts.end())
            {
                if (auto item = it->second.lock())
                {
                    (**layer).lyric = item->lyric;
                    return;
                }
            }
            parts.emplace(id, *layer);
        }
        // page: Add a locus command. In case of text presence change current target.
        void task(ansi::rule const& cmd)
        {
            test();
            auto& item = **layer;
            item.locus.push(cmd);
        }
        void meta(deco const& old_style) override
        {
            auto& item = **layer;
            item.style = parser::style;
        }
        void data(si32 count, grid const& proto) override
        {
            auto& item = **layer;
            item.lyric->splice(item.caret, count, proto);
            item.caret += count;
        }
        auto& current()       { return **layer; } // page: Access to the current paragraph.
        auto& current() const { return **layer; } // page: RO access to the current paragraph.
        auto  size()    const { return static_cast<si32>(batch.size()); }
    };

    // Derivative vt-parser example.
    struct derived_from_page
        : public page
    {
        template<class T>
        static void parser_config(T& vt)
        {
            page::parser_config(vt); // Inherit base vt-functionality.

            // Override vt-functionality.
            using namespace netxs::ansi;
            vt.intro[ctrl::TAB] = VT_PROC{ p->tabs(q.pop_all(ctrl::TAB)); };
        }

        derived_from_page (view utf8) {          ansi::parse(utf8, this);               }
        auto& operator  = (view utf8) { clear(); ansi::parse(utf8, this); return *this; }
        auto& operator += (view utf8) {          ansi::parse(utf8, this); return *this; }

        void tabs(si32) { log("Tabs are not supported"); }
    };

    class tone
    {
    public:

        #define PROP_LIST                       \
        X(kb_focus , "Keyboard focus indicator")\
        X(brighter , "Highlighter modificator") \
        X(shadower , "Darklighter modificator") \
        X(shadow   , "Light Darklighter modificator") \
        X(lucidity , "Global transparency")     \
        X(selector , "Selection overlay")       \
        X(bordersz , "Border size")

        #define X(a, b) a,
        enum prop { PROP_LIST count };
        #undef X

        //#define X(a, b) b,
        //text description[prop::count] = { PROP_LIST };
        //#undef X
        #undef PROP_LIST

        prop active  = prop::brighter;
        prop passive = prop::shadower;
    };
}

#endif // NETXS_RICHTEXT_HPP