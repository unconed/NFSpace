uniform samplerCube heightField;
uniform float sampleDistance;
uniform float inverseSampleDistance;
uniform float heightScale;

uniform vec3 faceTransform1;
uniform vec3 faceTransform2;
uniform vec3 faceTransform3;

void main() {

    // 3d world space coordinates of surface
    vec3 uvw0 = gl_TexCoord[0].xyz;
    // 2d uv coordinates relative to viewport/filter surface
    vec2 uv1 = gl_TexCoord[1].xy;

    // Recompose faceTransform matrix
    mat3 faceTransform = mat3(faceTransform1, faceTransform2, faceTransform3);
    
    // Prepare pixel jitter offsets in uvw space.
    vec3 xOffset = sampleDistance * faceTransform * vec3(1.0, 0.0, 0.0);
    vec3 yOffset = sampleDistance * faceTransform * vec3(0.0, 1.0, 0.0);

    // Calculate horizontal and vertical central differences.
    float xDifference = textureCube(heightField, uvw0 + xOffset).r
                      - textureCube(heightField, uvw0 - xOffset).r;
    float yDifference = textureCube(heightField, uvw0 + yOffset).r
                      - textureCube(heightField, uvw0 - yOffset).r;
    
    // Prepare (s,t,1) coordinate system (tangential flat plane to warp into sphere).
    float s = uv1.x;
    float t = uv1.y;
    float iw = inversesqrt(s * s + t * t + 1.0);
    float h = 1.0 + heightScale * textureCube(heightField, uvw0).r;
    
    // Precalculate values.
    float st = s * t;
    float iw2 = iw * iw;
    float iw3 = iw2 * iw;
    float hiw = h * iw;
    float hiw3 = h * iw3;
    
    // Prepare jacobian matrix.

    mat3 jacobian = mat3(
        hiw * (1.0 - s * s * iw2), -st * hiw3, -s * hiw3,
        -st * hiw3, hiw * (1.0 - t * t * iw2), -t * hiw3,
        heightScale * s * iw, heightScale * t * iw, heightScale * iw
    );

    // Calculate final normal
    vec3 normal = normalize(
        faceTransform * cross(
            jacobian * vec3(2.0, 0, -xDifference * inverseSampleDistance),
            jacobian * vec3(0, 2.0, -yDifference * inverseSampleDistance)
        )
    );

    float col = (normal.r - normal.g) *.707 * .866 + normal.b * .5;

//	gl_FragColor = vec4(normal * .5 + vec3(.5, .5, .5), 1.0);
	gl_FragColor = vec4(col, col, col, 1.0);
}
