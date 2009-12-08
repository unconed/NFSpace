uniform float invScale;
uniform vec2  planePosition;
uniform float planetRadius;
uniform float planetHeight;
uniform float skirtHeight;

uniform sampler2D heightMap;

uniform vec3 faceTransform1;
uniform vec3 faceTransform2;
uniform vec3 faceTransform3;

void main() {
    mat3 faceTransform = mat3(faceTransform1, faceTransform2, faceTransform3);
    vec2 planePoint = gl_Vertex.xy * invScale + planePosition;
    vec3 facePoint = faceTransform * vec3(planePoint, 1.0);
    vec3 spherePoint = normalize(facePoint) * (planetRadius + planetHeight * texture2D(heightMap, gl_MultiTexCoord0.xy).x + gl_Vertex.z * skirtHeight);
    
    gl_Position = gl_ModelViewProjectionMatrix * vec4(spherePoint, 1.0);

	gl_TexCoord[0] = gl_MultiTexCoord0;
}