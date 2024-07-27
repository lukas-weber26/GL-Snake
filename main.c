#include <assert.h>
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#define ball_radious 0.04

const char * vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "uniform vec2 transform;\n" 
    "void main()\n"
    "{\n"
    "gl_Position = -vec4(transform, 0.0, 0.0) + vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    "}\0";

const char * fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "FragColor = vec4(1.0,0.5,0.2,1.0);\n"
    "}\0";

void checkLink(unsigned int shaderProgram);
void checkSuccess(unsigned int vertexShader);
void framebuffer_size_callback(GLFWwindow * window, int width, int height);
void processInput(GLFWwindow * window);
void mouse_callback(GLFWwindow * window, double x_pos, double y_pos);
void check_food_intersection();

float random_float () {
    return (float) rand() / (float)(RAND_MAX/0.5);
}

typedef struct ball {
	float x;
	float y; 
	float v_x;
	float v_y; 
	float a_x;
	float a_y; 
	float m;
} ball;

typedef struct snake {
    struct snake * next_segment; 
    float x; 
    float y;
    float v_x;
    float v_y;
    float a_x; //not sure if these will be used 
    float a_y;
} snake;

snake s = {NULL, 0,0,0,0,0,0};
ball food = {0.75,0.75,0,0,0,0,1};

int main() {

    //these seem like pretty normal c library things! 
    glfwInit();
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //fortunately glew does not require too much crap

    GLFWwindow * window = glfwCreateWindow(800, 800, "First Window", NULL, NULL);
    if (window == NULL) {
	printf("GLFW Window fail.");
	glfwTerminate();
	exit(1);
    }

    //binds gl functions to the current window
    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, &mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwPollEvents(); //this seems to be an issue with wayland 
    
    glViewport(0, 0, 800, 800);
    
    //YEAH TURNS OUT THE POSITION OF THIS MATTERS A LOT. HAS TO BE AFTER THE WINDOW HAS BEEN CREATED!
    glewExperimental = GL_TRUE; 
    glewInit();

    #define number_of_vertices 36 

    float vertices[3*(number_of_vertices+2)] = {
	0.0, 0.0, 0.0,
    };

    for (int i = 1; i < number_of_vertices + 2; i++) {
	vertices[3*i] = ball_radious*cos(2*M_PI* ((float)(i-1)/(float)(number_of_vertices)));
	vertices[3*i + 1] = ball_radious*sin(2*M_PI* ((float)(i-1)/(float)(number_of_vertices)));
	vertices[3*i + 2] = 0.0;
    }

    //tell gl to generate vertex arrays and bind them
    unsigned int VAO;
    glGenVertexArrays(1,&VAO);
    glBindVertexArray(VAO);
     
    unsigned int VBO;
    glGenBuffers(1,&VBO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO); //you have to bind buffers to make the following gl operations affect said buffer
    glBufferData(GL_ARRAY_BUFFER,sizeof(vertices), vertices, GL_STATIC_DRAW); //note that GL_STATIC_DRAW means that this is not expected to change much
   
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER); 
    glShaderSource(vertexShader,1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    checkSuccess(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    checkSuccess(fragmentShader);

    unsigned int shaderProgram; 
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    checkLink(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    //final step here: Make the gpu interpret the shader program in a meaningful way!
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
    glEnable(GL_MULTISAMPLE);
    
    int transform_loc = glGetUniformLocation(shaderProgram,"transform");

    while(!glfwWindowShouldClose(window)) {
	processInput(window);
	
	glClearColor(0.14f, 0.14f, 0.14f, 0.5f);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(shaderProgram);
	glBindVertexArray(VAO);
    
	//draw snake 
	snake * draw_snake = &s;
	while (draw_snake) {
	    glUniform2f(transform_loc, draw_snake->x, draw_snake->y);
	    glDrawArrays(GL_TRIANGLE_FAN, 0, number_of_vertices + 2);
	    draw_snake = draw_snake -> next_segment;
	}

	glUniform2f(transform_loc, food.x, food.y);
	glDrawArrays(GL_TRIANGLE_FAN, 0, number_of_vertices + 2);
	
	glfwSwapBuffers(window);
	glfwPollEvents(); //this seems to be an issue with wayland 
	check_food_intersection();

    }
    
    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow * window, int width, int height) { 
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow * window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);	
    }
}

void checkSuccess(unsigned int vertexShader) {
    int success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);

    if (!success) {
	char infoLog[512];
	glGetShaderInfoLog(vertexShader,512,NULL,infoLog);
	printf("%s\n",infoLog);
	exit(0);
    }
}

void checkLink(unsigned int shaderProgram) {
    int success;
    glGetProgramiv(shaderProgram,GL_LINK_STATUS, &success);
    if (!success) {
	char out[512];
	glGetProgramInfoLog(shaderProgram,512,NULL,out);
	printf("%s",out);
	exit(0);
    }
}

void mouse_callback(GLFWwindow * window, double x_pos, double y_pos) {
    static float initial_x = 0;
    static float initial_y = 0;

    float current_x = (float) x_pos;
    float current_y = (float) y_pos;

    float diff_x = current_x - initial_x;
    float diff_y = current_y - initial_y;

    initial_x = current_x;
    initial_y = current_y;

    s.x -= diff_x*0.001;
    s.y += diff_y*0.001;

    //right
    if (s.x <= -0.85) {
        s.x = -0.85; 
    }
   
    //left
    if (s.x >= 0.95) {
        s.x = 0.95; 
    }
  
    //top
    if (s.y <= -0.23) {
        s.y = -0.23; 
    }
   
    //bottom
    if (s.y >= 0.95) {
        s.y = 0.95; 
    }
}

void check_snake_intersection() {
    snake * temp = s.next_segment;

    while (temp && temp->next_segment) {
	float x_dist = s.x - temp->x;
	float y_dist = s.y - temp->y;
	float absolute_dist = sqrt(x_dist*x_dist + y_dist*y_dist);
	if (absolute_dist < 0.03) {
	    glfwTerminate();
	    printf("You lost!\n");
	    exit(0);
	}

	temp = temp->next_segment;
    }
}

void update_position () {
    snake * temp = &s;
    float x_pos = temp->x;	
    float y_pos = temp->y;
    temp = temp -> next_segment;
    while (temp) {
    
	if (temp == s.next_segment && (sqrt((temp -> x - x_pos)*(temp -> x - x_pos) + (temp -> y - y_pos)*(temp -> y - y_pos)) < 0.05)) {
	    temp ->x += 0;
	    temp ->y += 0;	
	} else {
	    temp ->x += temp ->v_x;	
	    temp ->y += temp ->v_y;	
	}

	temp ->v_x = (x_pos - temp ->x)*0.1;
	temp ->v_y = (y_pos - temp ->y)*0.1;


	x_pos = temp->x;	
	y_pos = temp->y;
	temp = temp -> next_segment;
    }

    check_snake_intersection();

}

void check_food_intersection() {
    //s, food;    
    float dist_x = s.x - food.x;
    float dist_y = s.y - food.y;
    float absolute_dist = (sqrt(dist_x*dist_x + dist_y*dist_y));

    if (absolute_dist <= 0.05) {
	snake * new = malloc(sizeof(snake));
	new ->x = food.x;
	new ->y = food.y;
	new ->v_x= 0;
	new ->v_y = 0;
	new -> next_segment = NULL;

	snake * temp = &s;
	while (temp -> next_segment) {
	    temp = temp -> next_segment;
	}
	temp -> next_segment = new;
	
	food.x = 1.5*(random_float()-0.15); 
	food.y = random_float();

    }

    update_position();

}
