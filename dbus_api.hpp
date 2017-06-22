/* Copyright (C) 2011 The giomm Development Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.    See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#include <giomm.h>
#include <glibmm.h>
#include <iostream>

#include "NodeStateMachine.h"

namespace {

guint registered_id = 0;

static Glib::RefPtr<Gio::DBus::NodeInfo> introspection_data;
static Glib::ustring introspection_xml = "<node>"
                                         "    <interface name='org.genivi.SimpleNodeStateMachine'>"
                                         "        <method name='Shutdown'>"
                                         "            <arg type='u' name='error' direction='out'/>"
                                         "        </method>"
                                         "    </interface>"
                                         "</node>";


} // anonymous namespace

static void on_method_call(const Glib::RefPtr<Gio::DBus::Connection>& /* connection */,
                           const Glib::ustring& /* sender */,
                           const Glib::ustring& /* object_path */,
                           const Glib::ustring& /* interface_name */,
                           const Glib::ustring& method_name,
                           const Glib::VariantContainerBase& /* parameters */,
                           const Glib::RefPtr<Gio::DBus::MethodInvocation>& invocation)
{
    if (method_name == "Shutdown")
    {
        // Tell the node state machine
        Glib::Variant<guint32> response = Glib::Variant<guint32>::create(NsmcHandleShutdownRequest());

        invocation->return_value(Glib::VariantContainerBase::create_tuple(response));
    }
    else
    {
        // Non-existent method on the interface.
        Gio::DBus::Error error(Gio::DBus::Error::UNKNOWN_METHOD, "Method does not exist.");
        invocation->return_error(error);
    }
}

// This must be a global instance. See the InterfaceVTable documentation.
const Gio::DBus::InterfaceVTable interface_vtable(sigc::ptr_fun(&on_method_call));

void on_bus_acquired(const Glib::RefPtr<Gio::DBus::Connection>& connection,
                     const Glib::ustring& /* name */)
{
    // Export an object to the bus:

    // See https://bugzilla.gnome.org/show_bug.cgi?id=646417 about avoiding
    // the repetition of the interface name:
    try
    {
        registered_id = connection->register_object(
            "/org/glibmm/DBus/TestObject", introspection_data->lookup_interface(), interface_vtable);
    }
    catch (const Glib::Error& ex)
    {
        std::cerr << "Registration of object failed." << std::endl;
    }

    return;
}

void on_name_acquired(const Glib::RefPtr<Gio::DBus::Connection>& /* connection */,
                 const Glib::ustring& /* name */)
{
}

void on_name_lost(const Glib::RefPtr<Gio::DBus::Connection>& connection,
                  const Glib::ustring& /* name */)
{
    connection->unregister_object(registered_id);
}

int start_dbus()
{
    std::locale::global(std::locale(""));
    Gio::init();

    try
    {
        introspection_data = Gio::DBus::NodeInfo::create_for_xml(introspection_xml);
    }
    catch (const Glib::Error& ex)
    {
        std::cerr << "Unable to create introspection data: " << ex.what() << "." << std::endl;
        return 1;
    }

    auto a = sigc::ptr_fun(&on_bus_acquired);
    auto b = sigc::ptr_fun(&on_name_acquired);
    auto c = sigc::ptr_fun(&on_name_lost);

    const auto id = Gio::DBus::own_name(Gio::DBus::BusType::BUS_TYPE_SESSION, "org.genivi.SimpleNodeStateMachine",
        a, b, c
        );

    // Keep the service running until the process is killed:
//    auto loop = Glib::MainLoop::create();
//    loop->run();

//    Gio::DBus::unown_name(id);
    return 0;
}
