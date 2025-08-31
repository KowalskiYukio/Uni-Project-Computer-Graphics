#version 450 core

layout (location = 0) out vec4 fColour;

in vec4 col;
in vec3 nor;
in vec3 FragPosWorldSpace;
in vec2 tex;
in vec4 FragPosProjectedLightSpace;

uniform sampler2D shadowMap;

uniform vec3 lightDirection;
uniform vec3 lightColour;
uniform vec3 lightPos;
uniform vec3 camPos;
uniform sampler2D Texture;

float shadowOnFragment(vec4 FragPosProjectedLightSpace) {
	vec3 ndc = FragPosProjectedLightSpace.xyz / FragPosProjectedLightSpace.w;
	vec3 ss = (ndc + 1) * 0.5;

	float fragDepth = ss.z;
	if(fragDepth > 1 || fragDepth < 0)
		return 0.f;

	vec3 Nnor = normalize(nor);
	vec3 Ntolight = normalize(-lightDirection);
	float bias = max(0.0005 * (1.0 - dot(Nnor, Ntolight)), 0.00005);

    float currentDepth = ss.z;
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float litDepth = texture(shadowMap, ss.xy + vec2(x, y) * texelSize).r; 
            shadow += (fragDepth - bias) > litDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

	return shadow;
}

float CalculateDirectionalIllumination() {
	// ambient
	float ambient = 0.1f;

	// diffuse calculation
	vec3 Nnor = normalize(nor);
	vec3 Nto_light = normalize(-lightDirection);
	float diffuse = max(dot(Nnor, Nto_light), 0.f);

	// specular calculation
	vec3 Nfrom_light = normalize(lightDirection);
	vec3 NrefLight = reflect(Nfrom_light, Nnor);
	vec3 camDirection = camPos - FragPosWorldSpace;
	vec3 NcamDirection = normalize(camDirection);
	float brightness = max(dot(NcamDirection, NrefLight), 0.f);
	float specular = pow(brightness, 128);

	// combined calculation
	float shadow = shadowOnFragment(FragPosProjectedLightSpace);
	float phong = ambient + ((1.f - shadow) * (diffuse + specular));

	return phong;
}

float CalculatePositionalIllumination() {
	// ambient
	float ambient = 0.1f;

	// diffuse calculation
	vec3 Nnor = normalize(nor);
	vec3 Nto_light = normalize(lightPos - FragPosWorldSpace);
	float diffuse = max(dot(Nnor, Nto_light), 0.f);

	// specular calculation
	vec3 Nfrom_light = -Nto_light;
	vec3 NrefLight = reflect(Nfrom_light, Nnor);
	vec3 camDirection = camPos - FragPosWorldSpace;
	vec3 NcamDirection = normalize(camDirection);
	float specular = pow(max(dot(NcamDirection, NrefLight), 0.f), 128);

	// attenuation calculation
	float d = length(FragPosWorldSpace) - length(lightPos);
	float attenuation = 1 / (1.f + 0.05 * d + 0.002 * pow(d, 2));

	// combined calculation
	return (ambient + diffuse + specular) * attenuation;


}

float CalculateSpotIllumination() {
	// ambient
	float ambient = 0.1f;

	// diffuse calculation
	vec3 Nnor = normalize(nor);
	vec3 Nto_light = normalize(lightPos - FragPosWorldSpace);
	float diffuse = max(dot(Nnor, Nto_light), 0.f);

	// specular calculation
	vec3 Nfrom_light = -Nto_light;
	vec3 NrefLight = reflect(Nfrom_light, Nnor);
	vec3 camDirection = camPos - FragPosWorldSpace;
	vec3 NcamDirection = normalize(camDirection);
	float specular = pow(max(dot(NcamDirection, NrefLight), 0.f), 128);

	// attenuation calculation
	float d = length(FragPosWorldSpace) - length(lightPos);
	float attenuation = 1 / (1.f + 0.05 * d + 0.002 * pow(d, 2));

	// angles
	float cutoff = 15.f;
	float phi = cos(radians(cutoff));
	vec3 NspotDir = normalize(lightDirection);
	float theta = dot(Nfrom_light, NspotDir);

	// logic
	if(theta > phi)
		return (ambient + diffuse + specular) * attenuation;
	else
		ambient * attenuation;
}

void main()
{
	// Directional Illumination
	float phong = CalculateDirectionalIllumination();
//	float phong = CalculatePositionalIllumination();
//	float phong = CalculateSpotIllumination();
	
	vec4 texColour = texture(Texture, tex);

	fColour = vec4(col.rgb * texColour.rgb * phong * lightColour, texColour.a * col.a);
}
