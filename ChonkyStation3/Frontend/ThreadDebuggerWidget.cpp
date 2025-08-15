#include "ThreadDebuggerWidget.hpp"


ThreadDebuggerWidget::ThreadDebuggerWidget(PlayStation3* ps3, QWidget* parent) : QWidget(parent, Qt::Window), ps3(ps3) {
    ui.setupUi(this);
    
    timer.setInterval(10);
    connect(&timer, &QTimer::timeout, this, &ThreadDebuggerWidget::update);
    
    setWindowTitle("Thread Debugger");
    hide();
}

void ThreadDebuggerWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    
    timer.start();
}

void ThreadDebuggerWidget::closeEvent(QCloseEvent* event) {
    QWidget::closeEvent(event);
    
    timer.stop();
}

void ThreadDebuggerWidget::update() {
    // Create copies of the 2 thread lists to avoid possible synchronization issues
    auto ppu_threads = ps3->thread_manager.threads;
    auto spu_threads = ps3->spu_thread_manager.threads;

    // ***** PPU *****
    // Clear table
    ui.ppuTable->clear();
    
    ui.ppuTable->setSelectionMode(QAbstractItemView::NoSelection);
    ui.ppuTable->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui.ppuTable->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    // Get thread count
    size_t thread_count = ps3->thread_manager.threads.size();
    // Uncomment to ignore terminated threads
    for (int i = 0; i < ppu_threads.size(); i++) {
        if (ppu_threads[i].status == Thread::ThreadStatus::Terminated) thread_count--;
    }
    
    ui.ppuTable->setRowCount(thread_count);
    ui.ppuTable->setColumnCount(5);
    for (int i = 0; i < ui.ppuTable->rowCount(); i++)
        ui.ppuTable->setRowHeight(i, 20);
    ui.ppuTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui.ppuTable->setHorizontalHeaderLabels(QStringList({ "Name", "Status", "ID", "pc", "Wait reason"}));
    
    int row_idx = 0;
    for (int i = 0; i < ppu_threads.size(); i++) {
        auto& thread = ppu_threads[i];
        if (thread.status == Thread::ThreadStatus::Terminated) continue;
        
        setListItem(ui.ppuTable, row_idx, 0, thread.name);
        setListItem(ui.ppuTable, row_idx, 1, Thread::threadStatusToString(thread.status));
        setListItem(ui.ppuTable, row_idx, 2, std::format("{:d}", thread.id));
        setListItem(ui.ppuTable, row_idx, 3, std::format("{:08x}", thread.state.pc));
        setListItem(ui.ppuTable, row_idx, 4, thread.wait_reason);
        row_idx++;
    }
    
    // ***** SPU *****
    ui.spuTable->clear();
    
    ui.spuTable->setSelectionMode(QAbstractItemView::NoSelection);
    ui.spuTable->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui.spuTable->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    ui.spuTable->setRowCount(ps3->spu_thread_manager.threads.size());
    ui.spuTable->setColumnCount(4);
    for (int i = 0; i < ui.spuTable->rowCount(); i++)
        ui.spuTable->setRowHeight(i, 20);
    ui.spuTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui.spuTable->setHorizontalHeaderLabels(QStringList({ "Name", "Status", "ID", "pc"}));
    
    for (int i = 0; i < spu_threads.size(); i++) {
        auto& thread = spu_threads[i];
        setListItem(ui.spuTable, i, 0, thread.name);
        setListItem(ui.spuTable, i, 1, SPUThread::threadStatusToString(thread.status));
        setListItem(ui.spuTable, i, 2, std::format("{:d}", thread.id));
        setListItem(ui.spuTable, i, 3, std::format("{:08x}", thread.state.pc));
    }
}

void ThreadDebuggerWidget::setListItem(QTableWidget* table, int row, int column, std::string str) {
    QTableWidgetItem* item = new QTableWidgetItem();
    QWidget* widget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(widget);
    QLabel* label = new QLabel(widget);

    layout->setAlignment(Qt::AlignVCenter);
    layout->setContentsMargins(5, 0, 5, 0);
    label->setStyleSheet("color: white; font-size: 12px;");

    label->setText(QString::fromStdString(str));
    layout->addWidget(label);
    widget->setLayout(layout);

    table->setItem(row, column, item);
    table->setCellWidget(row, column, widget);
}
