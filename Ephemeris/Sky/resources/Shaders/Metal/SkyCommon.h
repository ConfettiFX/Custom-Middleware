/*
* Copyright (c) 2018-2019 Confetti Interactive Inc.
*
* This file is part of The-Forge
* (see https://github.com/ConfettiFX/The-Forge).
*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*/

constant int TRANSMITTANCE_INTEGRAL_SAMPLES = 500;
constant int INSCATTER_INTEGRAL_SAMPLES = 50;
constant int IRRADIANCE_INTEGRAL_SAMPLES = 32;
constant int INSCATTER_SPHERICAL_INTEGRAL_SAMPLES = 16;

constant float M_PI = 3.141592657f;

constant float Rg = 6360.0f;
constant float Rt = 6420.0f;
constant float RL = 6421.0f;

constant int TRANSMITTANCE_W = 256;
constant int TRANSMITTANCE_H = 64;

constant int SKY_W = 64;
constant int SKY_H = 16;

constant int RES_R = 32;
constant int RES_MU = 128;
constant int RES_MU_S = 32;
constant int RES_NU = 8;

constant float ISun = 100.0f;
