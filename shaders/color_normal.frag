#version 330

// in int gl_PrimitiveID; ?
in vec3 vNormal;
in vec2 line_pos;
in vec2 uv;
flat in vec2 line_start;

uniform vec3 color;
uniform vec3 light;
uniform bool unlit;
uniform bool stripple;
uniform vec2 viewport_size;

uniform sampler2D texture_sampler;


out vec4 fColor;


void main()
{
   if (unlit) {
       fColor = vec4(color, 1);
   } else {
       float diffusion = max(dot(normalize(vNormal), light), 0);
       fColor = vec4(diffusion * color, 1);
       //fColor = diffusion * texture(texture_sampler, uv);
   }

   if (stripple) {
       if ((int(gl_FragCoord.x + gl_FragCoord.y) & 1) == 0)
           discard;
   }

//   if (lineStripple) {
//       if (stripple) {
//           float  dist = length(line_pos - line_start);
//            fColor = vec4(color, 1);
//           float u_dashSize = 10;
//           float u_gapSize = 10;

//           if (fract(dist / (u_dashSize + u_gapSize)) > u_dashSize/(u_dashSize + u_gapSize))
//               discard;
//   }
}
