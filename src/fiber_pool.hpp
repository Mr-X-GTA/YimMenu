#pragma once

namespace baustelle
{
	class werkzeug_schicht
	{
	public:
		explicit fiber_pool(std::size_t num_fibers);
		~fiber_pool();

		void queue_job(std::function<void()> func);
		void execute_on_game_thread(std::function<void()> func);

		void fiber_tick();
		static void fiber_func();

		int get_total_fibers();
		int get_used_fibers();

		void reset();

	private:
		std::recursive_mutex m_mutex;
		std::stack<std::function<void()>> m_jobs;
		int m_num_fibers;
	};

	inline werkzeug_schicht* g_werkzeug{};  // Baustellen Werkzeug (Construction Tools)
}
