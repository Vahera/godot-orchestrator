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
#ifndef ORCHESTRATOR_PROPERTY_UTILS_H
#define ORCHESTRATOR_PROPERTY_UTILS_H

#include <godot_cpp/core/property_info.hpp>

using namespace godot;

namespace PropertyUtils
{
    /// Create a simple PropertyInfo for an object type with a given class name.
    /// @param p_name the property name
    /// @param p_class_name the object class name
    /// @return the property info structure
    PropertyInfo create_object(const String& p_name, const String& p_class_name = String());
};

#endif // ORCHESTRATOR_PROPERTY_UTILS_H