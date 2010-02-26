uniform vec4 tint;
uniform sampler2D heightMap;
uniform sampler2D normalMap;

void main() {
	gl_FragColor = vec4(texture2D(normalMap, gl_TexCoord[0].xy).xyz + 0.0*texture2D(heightMap, gl_TexCoord[0].xy).xyz, 1.0) * tint;
}
