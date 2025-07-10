#version 330

//in int gl_PrimitiveID;
uniform vec3 color;

out vec4 fColor;

void main()
{
//   if (selection) {

//   } else {

//   }
   fColor = vec4(color, 1);
}
