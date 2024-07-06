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
#include "script/nodes/scene/instantiate_scene.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource_loader.hpp>

class OScriptNodeInstantiateSceneInstance : public OScriptNodeInstance
{
    DECLARE_SCRIPT_NODE_INSTANCE(OScriptNodeInstantiateScene);
    Ref<PackedScene> _scene;

public:
    int step(OScriptExecutionContext& p_context) override
    {
        if (!_scene.is_valid())
        {
            _scene = ResourceLoader::get_singleton()->load(p_context.get_input(0));
            if (!_scene.is_valid())
            {
                p_context.set_error(GDEXTENSION_CALL_ERROR_INVALID_METHOD, "Failed to load scene");
                return -1;
            }
        }

        Node* scene_node = _scene->instantiate();
        p_context.set_output(0, scene_node);
        return 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OScriptNodeInstantiateScene::_get_property_list(List<PropertyInfo>* r_list) const
{
    r_list->push_back(PropertyInfo(Variant::STRING, "scene", PROPERTY_HINT_FILE, "*.scn,*.tscn"));
}

bool OScriptNodeInstantiateScene::_get(const StringName& p_name, Variant& r_value) const
{
    if (p_name.match("scene"))
    {
        r_value = _scene;
        return true;
    }
    return false;
}

bool OScriptNodeInstantiateScene::_set(const StringName& p_name, const Variant& p_value)
{
    if (p_name.match("scene"))
    {
        _scene = p_value;
        _notify_pins_changed();
        return true;
    }
    return false;
}

void OScriptNodeInstantiateScene::allocate_default_pins()
{
    create_pin(PD_Input, PT_Execution, "ExecIn");
    Ref<OScriptNodePin> scene = create_pin(PD_Input, PT_Data, "scene", Variant::STRING, _scene);
    scene->set_flag(OScriptNodePin::Flags::FILE);
    scene->set_file_types("*.scn,*.tscn; Scene Files");

    create_pin(PD_Output, PT_Execution, "ExecOut");
    create_pin(PD_Output, PT_Data, "scene_root", Variant::OBJECT);

    super::allocate_default_pins();
}

void OScriptNodeInstantiateScene::post_initialize()
{
    Ref<OScriptNodePin> scene = find_pin("scene", PD_Input);
    if (scene.is_valid())
    {
        _scene = scene->get_effective_default_value();

        // This is not currently persisted, so reset this
        scene->set_file_types("*.scn,*.tscn; Scene Files");
    }
    super::post_initialize();
}

String OScriptNodeInstantiateScene::get_tooltip_text() const
{
    return "Instantiates the specified scene.";
}

String OScriptNodeInstantiateScene::get_node_title() const
{
    return "Instantiate Scene";
}

String OScriptNodeInstantiateScene::get_icon() const
{
    return "PackedScene";
}

void OScriptNodeInstantiateScene::pin_default_value_changed(const Ref<OScriptNodePin>& p_pin)
{
    if (p_pin->get_pin_name().match("scene"))
        _scene = p_pin->get_default_value();

    super::pin_default_value_changed(p_pin);
}

StringName OScriptNodeInstantiateScene::resolve_type_class(const Ref<OScriptNodePin>& p_pin) const
{
    if (p_pin.is_valid() && p_pin->is_output() && !p_pin->is_execution())
    {
        Ref<PackedScene> packed_scene = ResourceLoader::get_singleton()->load(_scene);
        if (packed_scene.is_valid())
        {
            if (packed_scene->can_instantiate())
            {
                if (Node* node = packed_scene->instantiate())
                {
                    String class_name = node->get_class();
                    memdelete(node);
                    return class_name;
                }
            }
        }
    }
    return super::resolve_type_class(p_pin);
}

Ref<OScriptTargetObject> OScriptNodeInstantiateScene::resolve_target(const Ref<OScriptNodePin>& p_pin) const
{
    if (p_pin.is_valid() && p_pin->is_output() && !p_pin->is_execution())
    {
        Ref<PackedScene> packed_scene = ResourceLoader::get_singleton()->load(_scene);
        if (packed_scene.is_valid())
        {
            if (packed_scene->can_instantiate())
                return memnew(OScriptTargetObject(packed_scene->instantiate(), true));
        }
    }
    return super::resolve_target(p_pin);
}

OScriptNodeInstance* OScriptNodeInstantiateScene::instantiate()
{
    OScriptNodeInstantiateSceneInstance* i = memnew(OScriptNodeInstantiateSceneInstance);
    i->_node = this;
    return i;
}
