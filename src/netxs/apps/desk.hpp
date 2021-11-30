// Copyright (c) NetXS Group.
// Licensed under the MIT license.

#ifndef NETXS_APP_DESK_HPP
#define NETXS_APP_DESK_HPP

namespace netxs::events::userland
{
    struct desk
    {
        //EVENTPACK( desk, netxs::events::userland::root::custom )
        //{
        //    GROUP_XS( ui, input::hids ),

        //    SUBSET_XS( ui )
        //    {
        //        EVENT_XS( create  , input::hids ),
        //        GROUP_XS( split   , input::hids ),

        //        SUBSET_XS( split )
        //        {
        //            EVENT_XS( hz, input::hids ),
        //        };
        //    };
        //};
    };
}

// desk: Side Navigation Menu.
namespace netxs::app::desk
{
    using events = ::netxs::events::userland::desk;
    
    namespace
    {
        auto app_template = [](auto& data_src, auto const& utf8)
        {
            const static auto c4 = app::shared::c4;
            const static auto x4 = app::shared::x4;
            const static auto c5 = app::shared::c5;
            const static auto x5 = app::shared::x5;

            auto item_area = ui::pads::ctor(dent{ 1,0,1,0 }, dent{ 0,0,0,1 })
                    ->plugin<pro::fader>(x4, c4, 0ms)//150ms)
                    ->invoke([&](auto& boss)
                    {
                        auto data_src_shadow = ptr::shadow(data_src);
                        boss.SUBMIT_BYVAL(tier::release, hids::events::mouse::button::click::left, gear)
                        {
                            if (auto data_src = data_src_shadow.lock())
                            {
                                auto& inst = *data_src;
                                inst.SIGNAL(tier::preview, e2::form::layout::expose, inst);
                                auto& area = inst.base::area();
                                auto center = area.coor + (area.size / 2);
                                bell::getref(gear.id)->SIGNAL(tier::release, e2::form::layout::shift, center);  // Goto to the window.
                                gear.pass_kb_focus(inst);
                                gear.dismiss();
                            }
                        };
                        boss.SUBMIT_BYVAL(tier::release, hids::events::mouse::button::click::right, gear)
                        {
                            if (auto data_src = data_src_shadow.lock())
                            {
                                auto& inst = *data_src;
                                inst.SIGNAL(tier::preview, e2::form::layout::expose, inst);
                                auto& area = gear.area();
                                auto center = area.coor + (area.size / 2);
                                inst.SIGNAL(tier::preview, e2::form::layout::appear, center); // Pull window.
                                gear.pass_kb_focus(inst);
                                gear.dismiss();
                            }
                        };
                        boss.SUBMIT_BYVAL(tier::release, e2::form::state::mouse, hits)
                        {
                            if (auto data_src = data_src_shadow.lock())
                            {
                                data_src->SIGNAL(tier::release, e2::form::highlight::any, !!hits);
                            }
                        };
                    });
                auto label_area = item_area->attach(ui::fork::ctor());
                    auto mark_app = label_area->attach(slot::_1, ui::fork::ctor());
                        auto mark = mark_app->attach(slot::_1, ui::pads::ctor(dent{ 2,1,0,0 }, dent{ 0,0,0,0 }))
                                            ->attach(ui::item::ctor(ansi::fgc4(0xFF00ff00).add("‣"), faux));
                        auto app_label = mark_app->attach(slot::_2,
                                    ui::item::ctor(ansi::fgc(whitelt).add(utf8).mgl(0).wrp(wrap::off).jet(bias::left), true, true));
                    auto app_close_area = label_area->attach(slot::_2, ui::pads::ctor(dent{ 0,0,0,0 }, dent{ 0,0,1,1 }))
                                                    ->template plugin<pro::fader>(x5, c5, 150ms)
                                                    ->invoke([&](auto& boss)
                                                    {
                                                        auto data_src_shadow = ptr::shadow(data_src);
                                                        boss.SUBMIT_BYVAL(tier::release, hids::events::mouse::button::click::left, gear)
                                                        {
                                                            if (auto data_src = data_src_shadow.lock())
                                                            {
                                                                data_src->SIGNAL(tier::release, e2::form::proceed::detach, data_src);
                                                                gear.dismiss();
                                                            }
                                                        };
                                                    });
                        auto app_close = app_close_area->attach(ui::item::ctor("  ×  ", faux));
            return item_area;
        };
        auto apps_template = [](auto& data_src, auto& apps_map)
        {
            const static auto c3 = app::shared::c3;
            const static auto x3 = app::shared::x3;

            auto apps = ui::list::ctor();

            for (auto const& [class_id, inst_ptr_list] : *apps_map)
            {
                auto inst_id  = class_id;
                auto obj_desc = app::shared::objs_config[class_id].name;
                if (inst_ptr_list.size())
                {
                    auto item_area = apps->attach(ui::pads::ctor(dent{ 0,0,0,1 }, dent{ 0,0,1,0 }))
                                         ->template plugin<pro::fader>(x3, c3, 0ms)
                                         ->depend_on_collection(inst_ptr_list)
                                         ->invoke([&](auto& boss)
                                         {
                                             boss.mouse.take_all_events(faux);
                                             auto data_src_shadow = ptr::shadow(data_src);
                                             boss.SUBMIT_BYVAL(tier::release, hids::events::mouse::button::click::left, gear)
                                             {
                                                 if (auto data_src = data_src_shadow.lock())
                                                 {
                                                     sptr<registry_t> registry_ptr;
                                                     data_src->SIGNAL(tier::request, e2::bindings::list::apps, registry_ptr);
                                                     auto& app_list = (*registry_ptr)[inst_id];
                                                     // Rotate list forward.
                                                     app_list.push_back(app_list.front());
                                                     app_list.pop_front();
                                                     // Expose window.
                                                     auto& inst = *app_list.back();
                                                     inst.SIGNAL(tier::preview, e2::form::layout::expose, inst);
                                                     auto& area = inst.base::area();
                                                     auto center = area.coor + (area.size / 2);
                                                     bell::getref(gear.id)->
                                                     SIGNAL(tier::release, e2::form::layout::shift, center);  // Goto to the window.
                                                     gear.pass_kb_focus(inst);
                                                     gear.dismiss();
                                                 }
                                             };
                                             boss.SUBMIT_BYVAL(tier::release, hids::events::mouse::button::click::right, gear)
                                             {
                                                 if (auto data_src = data_src_shadow.lock())
                                                 {
                                                     sptr<registry_t> registry_ptr;
                                                     data_src->SIGNAL(tier::request, e2::bindings::list::apps, registry_ptr);
                                                     auto& app_list = (*registry_ptr)[inst_id];
                                                     // Rotate list forward.
                                                     app_list.push_front(app_list.back());
                                                     app_list.pop_back();
                                                     // Expose window.
                                                     auto& inst = *app_list.back();
                                                     inst.SIGNAL(tier::preview, e2::form::layout::expose, inst);
                                                     auto& area = inst.base::area();
                                                     auto center = area.coor + (area.size / 2);
                                                     bell::getref(gear.id)->
                                                     SIGNAL(tier::release, e2::form::layout::shift, center);  // Goto to the window.
                                                     gear.pass_kb_focus(inst);
                                                     gear.dismiss();
                                                 }
                                             };
                                         });
                        auto block = item_area->attach(ui::fork::ctor(axis::Y));
                            auto head_area = block->attach(slot::_1, ui::pads::ctor(dent{ 0,0,0,0 }, dent{ 0,0,1,1 }));
                                auto head = head_area->attach(ui::item::ctor(obj_desc, true));
                            auto list_pads = block->attach(slot::_2, ui::pads::ctor(dent{ 0,0,0,0 }, dent{ 0,0,0,0 }));
                    auto insts = list_pads->attach(ui::list::ctor())
                                            ->attach_collection(e2::form::prop::header, inst_ptr_list, app_template);
                }
            }
            return apps;
        };

        auto build = [](view v)
        {
            iota uibar_min_size = 4;
            iota uibar_full_size = 32;

            auto window = ui::cake::ctor();

            auto my_id = id_t{};

            auto user_info = utf::divide(v, ";");
            if (user_info.size() < 3) return window;
            auto& user_id___view = user_info[0];
            auto& user_name_view = user_info[1];
            auto& user_path_view = user_info[2];

            log("desk: user_id=", user_id___view, " user_name=", user_name_view, " user_path=", user_path_view);

            auto user = text{ user_name_view };
            auto path = text{ user_path_view };

            if (auto value = utf::to_int(user_id___view)) my_id = value.value();
            else return window;

            if (auto client = bell::getref(my_id))
            {
                // Taskbar Layout (PoC)
                auto client_shadow = ptr::shadow(client);

                auto menuitems_template = [client_shadow](auto& data_src, auto& apps_map)
                {
                    const static auto c3 = app::shared::c3;
                    const static auto x3 = app::shared::x3;

                    auto menuitems = ui::list::ctor();

                    if (auto client = client_shadow.lock())
                    {
                        auto current_default = decltype(e2::data::changed)::type{};
                        client->SIGNAL(tier::request, e2::data::changed, current_default);

                        for (auto const& [class_id, inst_ptr_list] : *apps_map)
                        {
                            auto id = class_id;
                            auto obj_desc = app::shared::objs_config[class_id].name;
                            auto selected = class_id == current_default;

                            auto item_area = menuitems->attach(ui::pads::ctor(dent{ 0,0,0,1 }, dent{ 0,0,1,0 }))
                                                    ->plugin<pro::fader>(x3, c3, 0ms)
                                                    ->invoke([&](auto& boss)
                                                    {
                                                        boss.mouse.take_all_events(faux);
                                                        boss.SUBMIT_BYVAL(tier::release, hids::events::mouse::button::click::left, gear)
                                                        {
                                                            if (auto client = bell::getref(gear.id))
                                                            {
                                                                client->SIGNAL(tier::release, e2::data::changed, id);
                                                                gear.dismiss();
                                                            }
                                                        };
                                                        boss.SUBMIT_BYVAL(tier::release, hids::events::mouse::button::dblclick::left, gear)
                                                        {
                                                            auto world_ptr = decltype(e2::config::whereami)::type{};
                                                            SIGNAL_GLOBAL(e2::config::whereami, world_ptr);
                                                            if (world_ptr)
                                                            {
                                                                static iota random = 0;
                                                                random = (random + 2) % 10;
                                                                auto offset = twod{ random * 2, random };
                                                                auto viewport = gear.area();
                                                                gear.slot.coor = viewport.coor + viewport.size / 8 + offset;
                                                                gear.slot.size = viewport.size * 3 / 4;
                                                                world_ptr->SIGNAL(tier::release, e2::form::proceed::createby, gear);
                                                            }
                                                        };
                                                    });
                                auto block = item_area->attach(ui::fork::ctor(axis::X));
                                    auto mark_area = block->attach(slot::_1, ui::pads::ctor(dent{ 1,1,0,0 }, dent{ 0,0,0,0 }));
                                        auto mark = mark_area->attach(ui::item::ctor(ansi::fgc4(selected ? 0xFF00ff00
                                                                                                         : 0xFF000000).add("██"), faux))
                                                    ->invoke([&](auto& boss)
                                                    {
                                                        auto client_id = client->id;
                                                        auto mark_shadow = ptr::shadow(boss.This());
                                                        client->SUBMIT_T_BYVAL(tier::release, e2::data::changed, boss.tracker, data)
                                                        {
                                                            auto selected = id == data;
                                                            if (auto mark = mark_shadow.lock())
                                                            {
                                                                mark->set(ansi::fgc4(selected ? 0xFF00ff00 : 0xFF000000).add("██"));
                                                                mark->deface();
                                                            }
                                                        };
                                                    });
                                    auto label_area = block->attach(slot::_2, ui::pads::ctor(dent{ 1,1,0,0 }, dent{ 0,0,0,0 }));
                                        auto label = label_area->attach(ui::item::ctor(ansi::fgc4(0xFFffffff).add(obj_desc), true, true));
                        }
                    }

                    return menuitems;
                };
                auto user_template = [my_id](auto& data_src, auto const& utf8)
                {
                    const static auto c3 = app::shared::c3;
                    const static auto x3 = app::shared::x3;

                    auto item_area = ui::pads::ctor(dent{ 1,0,0,1 }, dent{ 0,0,1,0 })
                                            ->plugin<pro::fader>(x3, c3, 150ms);
                        auto user = item_area->attach(ui::item::ctor(ansi::esc(" &").nil().add(" ")
                                    .fgc4(data_src->id == my_id ? rgba::color256[whitelt] : 0x00).add(utf8), true));
                    return item_area;
                };
                auto branch_template = [user_template](auto& data_src, auto& usr_list)
                {
                    auto users = ui::list::ctor()
                        ->attach_collection(e2::form::prop::name, *usr_list, user_template);
                    return users;
                };

                window->invoke([uibar_full_size, uibar_min_size](auto& boss) mutable
                    {
                        #ifdef _WIN32
                            auto current_default_sptr = std::make_shared<text>(app::shared::objs_lookup["CommandPrompt"]);
                            //auto current_default = app::shared::objs_lookup["PowerShell"];
                        #else
                            auto current_default_sptr = std::make_shared<text>(app::shared::objs_lookup["Term"]);
                        #endif
                        auto previous_default_sptr = std::make_shared<text>(*current_default_sptr);
                        auto subs_sptr = std::make_shared<subs>();

                        boss.SUBMIT_BYVAL(tier::release, e2::form::upon::vtree::attached, parent)
                        {
                            parent->SIGNAL(tier::release, e2::data::changed, *current_default_sptr);

                            parent->SUBMIT_T(tier::request, e2::data::changed, *subs_sptr, data)
                            {
                                if (current_default_sptr) data = *current_default_sptr;
                            };
                            parent->SUBMIT_T(tier::preview, e2::data::changed, *subs_sptr, data)
                            {
                                if (previous_default_sptr) data = *previous_default_sptr;
                            };
                            parent->SUBMIT_T(tier::release, e2::data::changed, *subs_sptr, data)
                            {
                                if (previous_default_sptr && current_default_sptr)
                                {
                                    auto new_default = data;
                                    if (*current_default_sptr != new_default)
                                    {
                                        *previous_default_sptr = *current_default_sptr;
                                        *current_default_sptr = new_default;
                                    }
                                }
                            };
                            parent->SUBMIT_T(tier::release, e2::form::upon::vtree::detached, *subs_sptr, p)
                            {
                                current_default_sptr.reset();
                                previous_default_sptr.reset();
                                subs_sptr.reset();
                            };
                        };
                    });
                    auto taskbar_viewport = window->attach(ui::fork::ctor(axis::X))
                                            ->invoke([](auto& boss)
                                            {
                                                boss.broadcast->SUBMIT(tier::request, e2::form::prop::viewport, viewport)
                                                {
                                                    viewport = boss.base::area();
                                                };
                                            });
                    auto taskbar = taskbar_viewport->attach(slot::_1, ui::fork::ctor(axis::Y))
                                        ->colors(whitedk, 0x60202020)
                                        ->plugin<pro::limit>(twod{ uibar_min_size,-1 }, twod{ uibar_min_size,-1 })
                                        ->plugin<pro::timer>()
                                        ->plugin<pro::acryl>()
                                        ->plugin<pro::cache>()
                                        ->invoke([&](auto& boss)
                                        {
                                            boss.mouse.template draggable<sysmouse::left>();
                                            auto boss_shadow = ptr::shadow(boss.This());
                                            auto size_config = std::make_shared<std::pair<iota, iota>>(uibar_full_size, uibar_min_size);
                                            boss.SUBMIT_BYVAL(tier::release, e2::form::drag::pull::_<sysmouse::left>, gear)
                                            {
                                                if (auto boss_ptr = boss_shadow.lock())
                                                {
                                                    auto& boss = *boss_ptr;
                                                    auto& [uibar_full_size, uibar_min_size] = *size_config;
                                                    auto& limits = boss.template plugins<pro::limit>();
                                                    auto lims = limits.get();
                                                    lims.min.x += gear.delta.get().x;
                                                    lims.max.x = uibar_full_size = lims.min.x;
                                                    limits.set(lims.min, lims.max);
                                                    boss.base::reflow();
                                                }
                                            };
                                            boss.SUBMIT_BYVAL(tier::release, e2::form::state::mouse, active)
                                            {
                                                if (auto boss_ptr = boss_shadow.lock())
                                                {
                                                    auto apply = [=](auto active)
                                                    {
                                                        if (auto boss_ptr = boss_shadow.lock())
                                                        {
                                                            auto& boss = *boss_ptr;
                                                            auto& [uibar_full_size, uibar_min_size] = *size_config;
                                                            auto& limits = boss.template plugins<pro::limit>();
                                                            auto size = active ? uibar_full_size : std::min(uibar_full_size, uibar_min_size);
                                                            auto lims = twod{ size,-1 };
                                                            limits.set(lims, lims);
                                                            boss.base::reflow();
                                                        }
                                                        return faux; // One shot call.
                                                    };
                                                    auto& timer = boss_ptr->template plugins<pro::timer>();
                                                    timer.pacify(faux);
                                                    if (active) apply(true);
                                                    else timer.actify(faux, MENU_TIMEOUT, apply);
                                                }
                                            };
                                            boss.broadcast->SUBMIT_T_BYVAL(tier::request, e2::form::prop::viewport, boss.tracker, viewport)
                                            {
                                                auto& [uibar_full_size, uibar_min_size] = *size_config;
                                                viewport.coor.x += uibar_min_size;
                                                viewport.size.x -= uibar_min_size;
                                            };
                                        });
                        auto apps_users = taskbar->attach(slot::_1, ui::fork::ctor(axis::Y, 0, 100));
                        {
                            const static auto c3 = app::shared::c3;
                            const static auto x3 = app::shared::x3;
                            const static auto c6 = app::shared::c6;
                            const static auto x6 = app::shared::x6;

                            auto world_ptr = decltype(e2::config::whereami)::type{};
                            SIGNAL_GLOBAL(e2::config::whereami, world_ptr);

                            auto apps_area = apps_users->attach(slot::_1, ui::fork::ctor(axis::Y));
                            {
                                auto label_pads = apps_area->attach(slot::_1, ui::pads::ctor(dent{ 0,0,1,1 }, dent{ 0,0,0,0 }))
                                                           ->plugin<pro::fader>(x3, c3, 150ms);
                                    auto label_bttn = label_pads->attach(ui::fork::ctor());
                                        auto label = label_bttn->attach(slot::_1,
                                            ui::item::ctor(ansi::fgc(whitelt).add("  ≡ "), faux, faux));
                                        auto bttn_area = label_bttn->attach(slot::_2, ui::fork::ctor());
                                            auto bttn_pads = bttn_area->attach(slot::_2, ui::pads::ctor(dent{ 2,2,0,0 }, dent{ 0,0,1,1 }))
                                                                      ->plugin<pro::fader>(x6, c6, 150ms);
                                                auto bttn = bttn_pads->attach(ui::item::ctor(">", faux));
                                auto applist_area = apps_area->attach(slot::_2, ui::pads::ctor(dent{ 0,0,1,0 }, dent{}))
                                                             ->attach(ui::cake::ctor());
                                    auto task_menu_area = applist_area->attach(ui::fork::ctor(axis::Y, 0, 0));
                                        auto menu_scrl = task_menu_area->attach(slot::_1, ui::rail::ctor(axes::ONLY_Y))
                                                                       ->colors(0x00, 0x00); //todo mouse events passthrough
                                            if (world_ptr)
                                            {
                                                auto menuitems = menu_scrl->attach_element(e2::bindings::list::apps, world_ptr, menuitems_template);
                                                auto tasks_scrl = task_menu_area->attach(slot::_2, ui::rail::ctor(axes::ONLY_Y))
                                                                                ->colors(0x00, 0x00); //todo mouse events passthrough
                                                auto apps = tasks_scrl->attach_element(e2::bindings::list::apps, world_ptr, apps_template);
                                            }
                                label_pads->invoke([&](auto& boss)
                                            {
                                                auto task_menu_area_shadow = ptr::shadow(task_menu_area);
                                                auto bttn_shadow = ptr::shadow(bttn);
                                                boss.SUBMIT_BYVAL(tier::release, hids::events::mouse::button::click::left, gear)
                                                {
                                                    if (auto bttn = bttn_shadow.lock())
                                                    if (auto task_menu_area = task_menu_area_shadow.lock())
                                                    {
                                                        auto state = task_menu_area->get_ratio();
                                                        bttn->set(state ? ">" : "<");
                                                        if (state) task_menu_area->config(0, 1);
                                                        else       task_menu_area->config(1, 0);
                                                        gear.dismiss();
                                                    }
                                                };
                                            });
                                apps_area->invoke([&](auto& boss)
                                            {
                                                auto task_menu_area_shadow = ptr::shadow(task_menu_area);
                                                auto bttn_shadow = ptr::shadow(bttn);
                                                boss.SUBMIT_BYVAL(tier::release, e2::form::state::mouse, active)
                                                {
                                                    if (!active)
                                                    if (auto bttn = bttn_shadow.lock())
                                                    if (auto task_menu_area = task_menu_area_shadow.lock())
                                                    {
                                                        if (auto state = task_menu_area->get_ratio())
                                                        {
                                                            bttn->set(">");
                                                            task_menu_area->config(0);
                                                        }
                                                    }
                                                };
                                            });
                                //todo make some sort of highlighting at the bottom and top
                                //scroll_bars_left(items_area, items_scrl);
                            }
                            auto users_area = apps_users->attach(slot::_2, ui::fork::ctor(axis::Y));
                            {
                                auto label_pads = users_area->attach(slot::_1, ui::pads::ctor(dent{ 0,0,1,1 }, dent{ 0,0,0,0 }))
                                                            ->plugin<pro::fader>(x3, c3, 150ms);
                                    auto label_bttn = label_pads->attach(ui::fork::ctor());
                                        auto label = label_bttn->attach(slot::_1,
                                            ui::item::ctor(ansi::fgc(whitelt).add("Users"), faux, faux));
                                        auto bttn_area = label_bttn->attach(slot::_2, ui::fork::ctor());
                                            auto bttn_pads = bttn_area->attach(slot::_2, ui::pads::ctor(dent{ 2,2,0,0 }, dent{ 0,0,1,1 }))
                                                                        ->plugin<pro::fader>(x6, c6, 150ms);
                                                auto bttn = bttn_pads->attach(ui::item::ctor("<", faux));
                                auto userlist_area = users_area->attach(slot::_2, ui::pads::ctor())
                                                                ->plugin<pro::limit>();
                                    if (world_ptr)
                                    {
                                        auto users = userlist_area->attach_element(e2::bindings::list::users, world_ptr, branch_template);
                                    }
                                    //auto users_rail = userlist_area->attach(ui::rail::ctor());
                                    //auto users = users_rail->attach_element(e2::bindings::list::users, world, branch_template);
                                //todo unify
                                bttn_pads->invoke([&](auto& boss)
                                            {
                                                auto userlist_area_shadow = ptr::shadow(userlist_area);
                                                auto bttn_shadow = ptr::shadow(bttn);
                                                auto state_ptr = std::make_shared<bool>(faux);
                                                boss.SUBMIT_BYVAL(tier::release, hids::events::mouse::button::click::left, gear)
                                                {
                                                    if (auto bttn = bttn_shadow.lock())
                                                    if (auto userlist = userlist_area_shadow.lock())
                                                    {
                                                        auto& state = *state_ptr;
                                                        state = !state;
                                                        bttn->set(state ? ">" : "<");
                                                        auto& limits = userlist->plugins<pro::limit>();
                                                        auto lims = limits.get();
                                                        lims.min.y = lims.max.y = state ? 0 : -1;
                                                        limits.set(lims, true);
                                                        userlist->base::reflow();
                                                    }
                                                };
                                            });
                            }
                        }
                        auto bttns_area = taskbar->attach(slot::_2, ui::fork::ctor(axis::X));
                        {
                            const static auto c2 = app::shared::c2;
                            const static auto x2 = app::shared::x2;
                            const static auto c1 = app::shared::c1;
                            const static auto x1 = app::shared::x1;

                            auto bttns = bttns_area->attach(slot::_1, ui::fork::ctor(axis::X));
                                auto disconnect_area = bttns->attach(slot::_1, ui::pads::ctor(dent{ 2,3,1,1 }))
                                                            ->plugin<pro::fader>(x2, c2, 150ms)
                                                            ->invoke([&](auto& boss)
                                                            {
                                                                boss.SUBMIT(tier::release, hids::events::mouse::button::click::left, gear)
                                                                {
                                                                    if (auto owner = base::getref(gear.id))
                                                                    {
                                                                        owner->SIGNAL(tier::release, e2::conio::quit, "taskbar: logout by button");
                                                                        gear.dismiss();
                                                                    }
                                                                };
                                                            });
                                    auto disconnect = disconnect_area->attach(ui::item::ctor("× Disconnect"));
                                auto shutdown_area = bttns->attach(slot::_2, ui::pads::ctor(dent{ 2,3,1,1 }))
                                                        ->plugin<pro::fader>(x1, c1, 150ms)
                                                        ->invoke([&](auto& boss)
                                                        {
                                                            boss.SUBMIT(tier::release, hids::events::mouse::button::click::left, gear)
                                                            {
                                                                //todo unify, see system.h:1614
                                                                #if defined(__APPLE__) || defined(__FreeBSD__)
                                                                auto path2 = "/tmp/" + path + ".sock";
                                                                ::unlink(path2.c_str());
                                                                #endif
                                                                os::exit(0, "taskbar: shutdown by button");
                                                            };
                                                        });
                                    auto shutdown = shutdown_area->attach(ui::item::ctor("× Shutdown"));
                        }
            }
            return window;
        };
    }

    app::shared::initialize builder{ "Desk", build };
}

#endif // NETXS_APP_DESK_HPP