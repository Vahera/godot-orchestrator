// This file is part of the Godot Orchestrator project.
//
// Copyright (c) 2023-present Vahera Studios LLC and its contributors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//		http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include "for.h"

#include "common/dictionary_utils.h"

class OScriptNodeForLoopInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeForLoop);

public:
    int get_working_memory_size() const override { return 1; }

    int step(OScriptNodeExecutionContext& p_context) override
    {
        // Break triggers node input port 1, check if that caused the step
        // If so we're done and we should exit.
        if (p_context.get_current_node_port() == 1)
            return 2;

        if (p_context.get_step_mode() == STEP_MODE_BEGIN)
            p_context.set_working_memory(0, p_context.get_input(0));
        else
            p_context.set_working_memory(0, int(p_context.get_working_memory()) + 1);

        // Check if we're done
        if (int(p_context.get_working_memory()) > int(p_context.get_input(1)))
            return 1;

        p_context.set_output(0, p_context.get_working_memory());
        return 0 | STEP_FLAG_PUSH_STACK_BIT;
    }

};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeForLoop::_get_property_list(List<PropertyInfo> *r_list) const
{
    r_list->push_back(PropertyInfo(Variant::BOOL, "with_break", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
    r_list->push_back(PropertyInfo(Variant::INT, "start", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
    r_list->push_back(PropertyInfo(Variant::INT, "end", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
}

bool OScriptNodeForLoop::_get(const StringName& p_name, Variant& r_value) const
{
    if (p_name.match("with_break"))
    {
        r_value = _with_break;
        return true;
    }
    else if (p_name.match("start"))
    {
        r_value = _start_index;
        return true;
    }
    else if (p_name.match("end"))
    {
        r_value = _end_index;
        return true;
    }
    return false;
}

bool OScriptNodeForLoop::_set(const StringName& p_name, const Variant& p_value)
{
    if (p_name.match("with_break"))
    {
        _with_break = p_value;
        _notify_pins_changed();
        return true;
    }
    else if (p_name.match("start"))
    {
        _start_index = p_value;
        return true;
    }
    else if (p_name.match("end"))
    {
        _end_index = p_value;
        return true;
    }
    return false;
}

void OScriptNodeForLoop::post_initialize()
{
    if (_with_break && !find_pin("aborted", PD_Output).is_valid())
        reconstruct_node();

    super::post_initialize();
}

void OScriptNodeForLoop::reallocate_pins_during_reconstruction(const Vector<Ref<OScriptNodePin>>& p_old_pins)
{
    super::reallocate_pins_during_reconstruction(p_old_pins);

    for (const Ref<OScriptNodePin>& pin : p_old_pins)
    {
        if (pin->is_input() && !pin->is_execution())
        {
            const Ref<OScriptNodePin> new_input = find_pin(pin->get_pin_name(), PD_Input);
            if (new_input.is_valid())
                new_input->set_default_value(pin->get_default_value());
        }
    }
}

void OScriptNodeForLoop::allocate_default_pins()
{
    create_pin(PD_Input, "ExecIn")->set_flags(OScriptNodePin::Flags::EXECUTION);
    create_pin(PD_Input, "first_index", Variant::INT, _start_index)->set_flags(OScriptNodePin::Flags::DATA);
    create_pin(PD_Input, "last_index", Variant::INT, _end_index)->set_flags(OScriptNodePin::Flags::DATA);

    if (_with_break)
        create_pin(PD_Input, "break")->set_flags(OScriptNodePin::Flags::EXECUTION | OScriptNodePin::SHOW_LABEL);

    create_pin(PD_Output, "loop_body")->set_flags(OScriptNodePin::Flags::EXECUTION | OScriptNodePin::Flags::SHOW_LABEL);
    create_pin(PD_Output, "index", Variant::INT)->set_flags(OScriptNodePin::Flags::DATA | OScriptNodePin::Flags::SHOW_LABEL);
    create_pin(PD_Output, "completed")->set_flags(OScriptNodePin::Flags::EXECUTION | OScriptNodePin::SHOW_LABEL);

    if (_with_break)
        create_pin(PD_Output, "aborted")->set_flags(OScriptNodePin::Flags::EXECUTION | OScriptNodePin::SHOW_LABEL);

    super::allocate_default_pins();
}

String OScriptNodeForLoop::get_tooltip_text() const
{
    return "Executes the 'Loop Body' for each index between the first and last index (inclusive).";
}

String OScriptNodeForLoop::get_node_title() const
{
    return vformat("For Loop%s", _with_break ? " With Break" : "");
}

String OScriptNodeForLoop::get_icon() const
{
    return "Loop";
}

void OScriptNodeForLoop::get_actions(List<Ref<OScriptAction>>& p_action_list)
{
    if (_with_break)
    {
        Callable callable = callable_mp(this, &OScriptNodeForLoop::_set_with_break).bind(false);
        p_action_list.push_back(memnew(OScriptAction("Remove break pin", "Remove", callable)));
    }
    else
    {
        Callable callable = callable_mp(this, &OScriptNodeForLoop::_set_with_break).bind(true);
        p_action_list.push_back(memnew(OScriptAction("Add break pin", "Add", callable)));
    }
    super::get_actions(p_action_list);
}

OScriptNodeInstance* OScriptNodeForLoop::instantiate(OScriptInstance* p_instance)
{
    OScriptNodeForLoopInstance* i = memnew(OScriptNodeForLoopInstance);
    i->_node = this;
    i->_instance = p_instance;
    return i;
}

void OScriptNodeForLoop::initialize(const OScriptNodeInitContext& p_context)
{
    if (p_context.user_data)
    {
        const Dictionary& data = p_context.user_data.value();
        if (data.has("with_break"))
            _with_break = data["with_break"];
    }
    super::initialize(p_context);
}

void OScriptNodeForLoop::_set_with_break(bool p_break_status)
{
    _with_break = p_break_status;
    reconstruct_node();
}