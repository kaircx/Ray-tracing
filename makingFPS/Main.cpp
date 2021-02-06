#include <Siv3D.hpp>
#include <numbers>

class Player {
public:
	Vec2 pos;
	double pi = 0;
	int vel = 2;
	double turnvel = std::numbers::pi /50 ;
	Player(Vec2 p) :pos(p.x,p.y){}
	void update() {
		if (KeyUp.pressed()) {
			pos.x += vel * std::cos(pi);
			pos.y += vel * std::sin(pi);
		}
		if (KeyDown.pressed()) {
			pos.x -= vel * std::cos(pi);
			pos.y -= vel * std::sin(pi);
		}
		if (KeyRight.pressed()) {
			pi += turnvel;
		}
		if (KeyLeft.pressed()) {
			pi -= turnvel;
		}
	}
	void draw(){
		Circle(pos.x, pos.y, 30.0).draw(Color(0, 0, 255));
	}
 };


class LimitLine {
public:
	Vec2 begin_pos;
	Vec2 end_pos;
	LimitLine(Vec2 _p, Vec2 _v) :begin_pos(_p), end_pos(_v) {
		Line(begin_pos.x, begin_pos.y, end_pos.x, end_pos.y).draw();
	}
	LimitLine(Vec2 _p, double _pi, double _length) :begin_pos(_p){
		end_pos = { _p.x + _length * std::cos(_pi),_p.y +_length * std::sin(_pi) };
		Line(begin_pos.x, begin_pos.y, end_pos.x, end_pos.y).draw();
	}
	double length() {
		double x = (end_pos - begin_pos).x;
		double y = (end_pos - begin_pos).y;
		return std::sqrt(x * x + y * y);
	}
	Vec2 v() {
		double x = (end_pos - begin_pos).x;
		double y = (end_pos - begin_pos).y;
		return { x / length(),y / length() };
	}

};

class Eye {
public:
	std::vector<LimitLine> limitlines;
	double eye_range = std::numbers::pi / 2;
	int eye_number = 10;
	int eye_length = 150;

	Eye(Vec2 pos, double pi) {
		for (int i = 0; i < eye_number; i++) {
			double theta = (pi - eye_range / 2) + i * (eye_range / eye_number);
			limitlines.push_back(LimitLine(pos, theta, eye_length));
		}

	}

};

Vec2 intersection(LimitLine a, LimitLine b) {
	double t1 = (a.end_pos.y - a.begin_pos.y)/(a.end_pos.x - a.begin_pos.x);
	double t2 = (b.end_pos.y - b.begin_pos.y) / (b.end_pos.x - b.begin_pos.x);
	double x = (t1 * a.begin_pos.x - t2 * b.begin_pos.x - a.begin_pos.x + b.begin_pos.y) / (t1 - t2);
	double y = t2 * (b.begin_pos.x - x) + b.begin_pos.y;
	return {x,y};
};


void Main() {
	Player Player({0,0});
	while (System::Update()) {
		Eye eye(Player.pos, Player.pi);
		LimitLine a({ 100,100 }, { 300, 400 });
		//LimitLine b({ 300,400 }, { 300,200 });
		//LimitLine c({ 300,200 }, { 100, 100 });
		Player.update();
		Player.draw();
		Circle(intersection(eye.limitlines[5],a), 5).draw(Palette::Orange);
	}
}