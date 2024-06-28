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
#include "property.h"

#include "common/dictionary_utils.h"
#include "script/script.h"

OScriptNodeProperty::OScriptNodeProperty()
{
    _flags = ScriptNodeFlags::NONE;

    _property.hint = PROPERTY_HINT_NONE;
    _property.usage = PROPERTY_USAGE_DEFAULT;
}

void OScriptNodeProperty::_bind_methods()
{
    BIND_ENUM_CONSTANT(CallMode::CALL_SELF)
    BIND_ENUM_CONSTANT(CallMode::CALL_INSTANCE)
    BIND_ENUM_CONSTANT(CallMode::CALL_NODE_PATH)
}

void OScriptNodeProperty::_get_property_list(List<PropertyInfo>* r_list) const
{
    uint32_t usage = PROPERTY_USAGE_STORAGE;
    if (!_node_path.is_empty())
    {
        usage |= PROPERTY_USAGE_EDITOR;
        usage |= PROPERTY_USAGE_READ_ONLY;
    }

    const String modes = "Self,Instance,Node Path";
    r_list->push_back(PropertyInfo(Variant::INT, "mode", PROPERTY_HINT_ENUM, modes, usage));

    // todo: deprecated, remove in a future release
    r_list->push_back(PropertyInfo(Variant::STRING, "target_class", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
    r_list->push_back(PropertyInfo(Variant::STRING, "property_name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
    r_list->push_back(PropertyInfo(Variant::STRING, "property_hint", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));

    r_list->push_back(PropertyInfo(Variant::DICTIONARY, "property", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
    r_list->push_back(PropertyInfo(Variant::NODE_PATH, "node_path", PROPERTY_HINT_NONE, "", usage));
}

bool OScriptNodeProperty::_get(const StringName& p_name, Variant& r_value) const
{
    if (p_name.match("mode"))
    {
        r_value = _call_mode;
        return true;
    }
    else if (p_name.match("target_class"))
    {
        r_value = _base_type;
        return true;
    }
    else if (p_name.match("property_name"))
    {
        r_value = _property.name;
        return true;
    }
    else if (p_name.match("property_hint"))
    {
        r_value = _property.hint_string;
        return true;
    }
    else if (p_name.match("property"))
    {
        r_value = DictionaryUtils::from_property(_property);
        return true;
    }
    else if (p_name.match("node_path"))
    {
        r_value = _node_path;
        return true;
    }
    return false;
}

bool OScriptNodeProperty::_set(const StringName& p_name, const Variant& p_value)
{
    if (p_name.match("mode"))
    {
        _call_mode = CallMode(int(p_value));
        return true;
    }
    else if (p_name.match("target_class"))
    {
        _base_type = p_value;
        return true;
    }
    else if (p_name.match("property_name"))
    {
        if (!_has_property)
            _property.name = p_value;
        return true;
    }
    else if (p_name.match("property_hint"))
    {
        if (!_has_property)
            _property.hint_string = p_value;
        return true;
    }
    else if (p_name.match("property"))
    {
        _property = DictionaryUtils::to_property(p_value);
        if (!_property.name.is_empty())
            _has_property = true;
        return true;
    }
    else if (p_name.match("node_path"))
    {
        _node_path = p_value;
        return true;
    }
    return false;
}

void OScriptNodeProperty::post_initialize()
{
    for (const Ref<OScriptNodePin>& pin : find_pins())
    {
        if (pin->get_pin_name().match(_property.name))
            _property.type = pin->get_type();
    }

    if (!_has_property)
    {
        // Upgrade the pin to have the property details
        if (_call_mode == CALL_INSTANCE || _call_mode == CALL_SELF)
        {
            // Get the property details from the class
            const String base = CALL_INSTANCE ? _base_type : get_orchestration()->get_base_type();
            TypedArray<Dictionary> properties = ClassDB::class_get_property_list(base);
            for (int i = 0; i < properties.size(); i++)
            {
                const Dictionary& prop = properties[i];
                if (prop.has("name") && _property.name.match(prop["name"]))
                {
                    _property = DictionaryUtils::to_property(prop);
                    _has_property = true;
                    break;
                }
            }
            reconstruct_node();
        }
        // todo: for node paths these need to be re-created somehow.
    }

    super::post_initialize();
}

String OScriptNodeProperty::get_icon() const
{
    return "MemberProperty";
}

void OScriptNodeProperty::initialize(const OScriptNodeInitContext& p_context)
{
    ERR_FAIL_COND_MSG(!p_context.property, "A property node requires a PropertyInfo");

    const PropertyInfo& pi = p_context.property.value();

    _property = pi;
    _has_property = true;

    if (p_context.node_path)
    {
        // This happens when a user drags a property from the inspector for a given node
        // in the current scene.
        _call_mode = CALL_NODE_PATH;
        _node_path = p_context.node_path.value();
    }
    else if (p_context.class_name)
    {
        _call_mode = CALL_INSTANCE;
        _base_type = p_context.class_name.value();
    }
    else
    {
        _call_mode = CALL_SELF;
    }
    super::initialize(p_context);
}
