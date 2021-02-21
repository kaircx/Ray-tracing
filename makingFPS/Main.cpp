#include <Siv3D.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

class Eye {
public:
	static constexpr double eye_range = std::numbers::pi / 2.2;
	static constexpr double eye_length = 80;
	static constexpr int eye_number = 200;
	std::vector<std::pair<Line, double>> lines;
	const Vec2& pos_;
	const double& theta_;

	Eye(const Vec2& pos, const double& theta) : pos_(pos), theta_(theta) {
		update();
	}

	void update() {
		lines.clear();
		lines.reserve(eye_number);
		for (std::size_t i = 0; i < eye_number; ++i) {
			const double phi =
				(theta_ - eye_range / 2) + i * (eye_range / eye_number);
			lines.emplace_back(
				Line(pos_, eye_length * Vec2(std::cos(phi), std::sin(phi)) + pos_),
				std::abs(phi - theta_));
		}
	}

	void draw() {
		for (const auto& line : lines) {
			line.first.draw();
		}
	}
};

class Player {
public:
	Vec2 pos;
	double theta = (std::numbers::pi / 8) * 4;
	double vel = 1.5;//1
	Eye eye;
	double turnvel = std::numbers::pi / 60;//80
	std::vector<Line> map_;
	const Audio audio;
	Player(Vec2 p, const std::vector<Line>& map) : audio(U"./nc92986.wav", Arg::loop = true),pos(p.x, p.y), eye(pos, theta), map_(map) {
	
	}

	void update() {

		const auto tmp_pos = pos;
		if (KeyUp.pressed()) {
			audio.play();
			pos.x += vel * std::cos(theta);
			pos.y += vel * std::sin(theta);
		}
		else if (KeyDown.pressed()) {
			audio.play();
			pos.x -= vel * std::cos(theta);
			pos.y -= vel * std::sin(theta);
		}
		else if (std::any_of(map_.cbegin(), map_.cend(), [this](const auto& a) { return a.intersects(pos); })) { pos = tmp_pos; }

		else if (KeyRight.pressed()) {
			//audio.play();
			theta += turnvel;
		}else if (KeyLeft.pressed()) {
			//audio.play();
			theta -= turnvel;
		}
		else audio.stop();
		

		eye.update();
	}
	void draw() {
		Circle(pos.x, pos.y, 5.0).draw(Color(0, 0, 255));
		eye.draw();
	}
};

std::vector<std::optional<std::pair<Vec2, double>>> makefocus(const Player& Player,
	const std::vector<Line>& walls) {

	const Vec2 light = Player.pos;

	std::vector<std::optional<std::pair<Vec2, double>>> focus;
	for (const auto& l : Player.eye.lines) {
		const auto& line = l.first;
		std::vector<std::pair<Vec2, double>> tmpfocus;
		for (const auto& wall : walls) {
			if (wall.intersectsAt(line).has_value()) {
				const auto pos = wall.intersectsAt(line).value().asPoint();
				tmpfocus.emplace_back(pos,
					0.05 + 1000.0 * std::abs(wall.vector().normalized().cross(
						(light - pos).normalized())) / std::pow(Geometry2D::Distance(pos, light), 2));
				Circle(wall.intersectsAt(line).value().asPoint(), 5)
					.draw(Palette::Orange);
			}
		}
		if (tmpfocus.empty()) {
			focus.push_back(std::nullopt);
		}
		else {
			auto itr = std::min_element(tmpfocus.cbegin(), tmpfocus.cend(),
				[&Player](const auto& a, const auto& b) {
					return	Geometry2D::Distance(a.first, Player.pos) <
						Geometry2D::Distance(b.first, Player.pos);
				});
			focus.push_back(*itr);
		}
	}
	return focus;
}

void drawFPSview(const std::vector<std::optional<std::pair<Vec2, double>>>& focus, const Player& Player, double cellsize, int height, int width) {
	for (int i = 0; i < Player.eye.lines.size(); i++) {
		const double window_width = Window::ClientSize().x;
		const double window_height = Window::ClientSize().y;
		const double tmp = ((window_width / 4) * 3 / Player.eye.lines.size()) * i;

		if (focus[i].has_value()) {
			const auto dist = Geometry2D::Distance(Player.pos, focus[i].value().first) *

				std::cos(Player.eye.lines[i].second);

			constexpr auto wall_height = 5000;
			int a = focus[i].value().first.y;
			int b = focus[i].value().first.x;
			if (a <= cellsize * 2 && b <= cellsize * 2) {
				Line(window_width / 4 + tmp, window_height / 2 - wall_height / dist,
					window_width / 4 + tmp, window_height / 2 + wall_height / dist)
					.draw(8, HSV(241, 1, focus[i].value().second));
			}
			else if (a >= cellsize * (height - 2) && b >= cellsize * (width - 2)) {
				Line(window_width / 4 + tmp, window_height / 2 - wall_height / dist,
					window_width / 4 + tmp, window_height / 2 + wall_height / dist)
					.draw(8, Palette::Red, HSV(0, 1, focus[i].value().second));
			}
			else if (a % static_cast<int>(cellsize) == 0 && b % static_cast<int>(cellsize) == 0) {
				Line(window_width / 4 + tmp, window_height / 2 - wall_height / dist,
					window_width / 4 + tmp, window_height / 2 + wall_height / dist)
					.draw(8, Palette::Orange, HSV(32, 82, focus[i].value().second));
			}
			else {
				Line(window_width / 4 + tmp, window_height / 2 - wall_height / dist,
					window_width / 4 + tmp, window_height / 2 + wall_height / dist)
					.draw(8, HSV(0, 0, focus[i].value().second));
			}
		}
	}
	if (std::hypot(cellsize * (width - 2) - Player.pos.x, cellsize * (height - 2) - Player.pos.y) < cellsize) {
		const Font font(36, Typeface::Bold);
		font(U"Goal!").draw(Window::ClientSize().x / 4, Window::ClientSize().y / 2);
	}
}

std::vector<Line> makemap(int height, int width, double cell_size) {
	std::vector<Line> walls;

	std::vector<std::vector<int>> map;
	map.resize(height);
	for (auto& m : map) {
		m.resize(width);
		std::fill(m.begin(), m.end(), 1);
	}

	enum class direction {
		up,
		down,
		right,
		left,
		size
	};

	int digg_pos_x = 1;
	int digg_pos_y = 1;
	map[digg_pos_y][digg_pos_x] = 0;
	std::unordered_map<direction, bool> end_flags;
	for (auto dir = static_cast<int>(direction::up); dir < static_cast<int>(direction::size); ++dir) end_flags[static_cast<direction>(dir)] = false;

	int a = 0;
	while (a < 10000) {
		a++;
		const auto dir = static_cast<direction>(Random(0, 3));

		switch (dir) {
		case direction::up:
			digg_pos_y -= 2;
			if (digg_pos_y > 0 && map[digg_pos_y][digg_pos_x] == 1) {
				map[digg_pos_y][digg_pos_x] = 0;
				map[digg_pos_y + 1][digg_pos_x] = 0;
				for (auto& e : end_flags) e.second = false;
			}
			else {
				end_flags[direction::up] = true;
				digg_pos_y += 2;
			}
			break;
		case direction::down:
			digg_pos_y += 2;
			if (digg_pos_y < height && map[digg_pos_y][digg_pos_x] == 1) {
				map[digg_pos_y][digg_pos_x] = 0;
				map[digg_pos_y - 1][digg_pos_x] = 0;
				for (auto& e : end_flags) e.second = false;
			}
			else {
				end_flags[direction::down] = true;
				digg_pos_y -= 2;
			}
			break;
		case direction::right:
			digg_pos_x += 2;
			if (digg_pos_x < width && map[digg_pos_y][digg_pos_x] == 1) {
				map[digg_pos_y][digg_pos_x] = 0;
				map[digg_pos_y][digg_pos_x - 1] = 0;
				for (auto& e : end_flags) e.second = false;
			}
			else {
				end_flags[direction::right] = true;
				digg_pos_x -= 2;
			}
			break;
		case direction::left:
			digg_pos_x -= 2;
			if (digg_pos_x > 0 && map[digg_pos_y][digg_pos_x] == 1) {
				map[digg_pos_y][digg_pos_x] = 0;
				map[digg_pos_y][digg_pos_x + 1] = 0;
				for (auto& e : end_flags) e.second = false;
			}
			else {
				end_flags[direction::left] = true;
				digg_pos_x += 2;
			}
			break;
		}
		int Random_x = std::floor(Random(2, width) / 2) - 1;
		int Random_y = std::floor(Random(2, height) / 2) - 1;

		if (std::all_of(end_flags.cbegin(), end_flags.cend(), [](const auto& e) { return e.second; }) && map[Random_y * 2 + 1][Random_x * 2 + 1] == 0) {
			digg_pos_x = Random_x * 2 + 1;
			digg_pos_y = Random_y * 2 + 1;
			for (auto& e : end_flags) e.second = false;
		}
	}

	for (auto m = map.begin(); m != map.end(); m++) {
		for (auto n = m->begin(); n != m->end(); n++) {
			if (m == map.begin() || m == map.end() - 1 || n == m->begin() || n == m->end() - 1) *n = 1;
		}
	}



	for (auto i = 0; i < height; i++) {
		Vec2 p;
		p.y = i * cell_size;
		for (auto j = 0; j < width; j++) {
			p.x = j * cell_size;
			if (map[i][j] == 1) {
				walls.emplace_back(p.x, p.y, p.x + cell_size, p.y);
				walls.emplace_back(p.x, p.y, p.x, p.y + cell_size);
				walls.emplace_back(p.x + cell_size, p.y + cell_size, p.x + cell_size, p.y);
				walls.emplace_back(p.x + cell_size, p.y + cell_size, p.x, p.y + cell_size);
			}
		}
	}
	return walls;
}

void drawmap(const std::vector<Line>& walls) {
	for (const auto& wall : walls) wall.draw();
}

void Main() {
	Window::SetStyle(WindowStyle::Sizable);
	Scene::SetScaleMode(ScaleMode::ResizeFill);
	constexpr int height = 39;
	constexpr int width = 13;
	const double cell_size = (Window::ClientSize().x / 4) / width;
	const Audio audio(U"./nc234276.wav", Arg::loop = true);

	const auto map = makemap(height, width, cell_size);
	Player Player({ 20, 20 }, map);
	audio.play();
	while (System::Update()) {
		const auto x = Window::ClientSize().x;
		const auto y = Window::ClientSize().y;
		Rect(x / 4, 0, x, y).draw(Arg::top = Palette::Lightblue, Arg::bottom = Palette::Black);
		Rect(x / 4, y / 2, x, y).draw(Arg::top = Palette::Black, Arg::bottom = Palette::Green);

		Player.update();
		Player.draw();
		drawmap(map);
		drawFPSview(makefocus(Player, map), Player, cell_size, height, width);
	}
}
