uniform vec4  stuvScale;
uniform vec4  stuvPosition;
uniform float planetRadius;
uniform float planetHeight;
uniform float skirtHeight;

uniform sampler2D heightMap;

uniform vec3 faceTransform1;
uniform vec3 faceTransform2;
uniform vec3 faceTransform3;

void main() {
    mat3 faceTransform = mat3(faceTransform1, faceTransform2, faceTransform3);
    
    // Vector is laid out as (s, t, u, v)
    vec4 stuvPoint = vec4(gl_Vertex.xy, gl_Vertex.xy) * stuvScale + stuvPosition;
    
    vec3 facePoint = faceTransform * vec3(stuvPoint.xy, 1.0);
    vec3 spherePoint = normalize(facePoint) * (planetRadius + planetHeight * texture2D(heightMap, stuvPoint.zw).x + gl_Vertex.z * skirtHeight);
    
    gl_Position = gl_ModelViewProjectionMatrix * vec4(spherePoint, 1.0);

	gl_TexCoord[0] = vec4(stuvPoint.zw, 0.0, 0.0);
}