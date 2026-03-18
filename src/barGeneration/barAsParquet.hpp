#pragma once
#include "commons.hpp"
#include <arrow/api.h>
#include <arrow/io/api.h>  // optional but common
#include <arrow/io/file.h> // REQUIRED for FileOutputStream
#include <memory>
#include <parquet/arrow/writer.h>
#include <parquet/properties.h>
#include <vector>

// Could maybe add better implementation with a template here, see later if
// scales
inline arrow::Result<std::shared_ptr<arrow::RecordBatch>> convertToArrow(
  const std::vector<bar>& bars)
{
  arrow::TimestampBuilder time_builder(
    arrow::timestamp(arrow::TimeUnit::NANO), arrow::default_memory_pool());
  arrow::DoubleBuilder start_bid_builder;
  arrow::DoubleBuilder end_bid_builder;
  arrow::DoubleBuilder high_bid_builder;
  arrow::DoubleBuilder low_bid_builder;
  arrow::DoubleBuilder avg_bid_builder;
  arrow::DoubleBuilder avg_bid_size_builder;
  arrow::DoubleBuilder start_ask_builder;
  arrow::DoubleBuilder end_ask_builder;
  arrow::DoubleBuilder high_ask_builder;
  arrow::DoubleBuilder low_ask_builder;
  arrow::DoubleBuilder avg_ask_builder;
  arrow::DoubleBuilder avg_ask_size_builder;
  arrow::Int32Builder  trade_quantity_builder;
  arrow::Int32Builder  bid_quote_count_builder;
  arrow::Int32Builder  ask_quote_count_builder;

  for (const auto& b : bars) {
    ARROW_RETURN_NOT_OK(time_builder.Append(b.time.time_since_epoch().count()));
    ARROW_RETURN_NOT_OK(start_bid_builder.Append(b.startBid));
    ARROW_RETURN_NOT_OK(end_bid_builder.Append(b.endBid));
    ARROW_RETURN_NOT_OK(high_bid_builder.Append(b.highBid));
    ARROW_RETURN_NOT_OK(low_bid_builder.Append(b.lowBid));
    ARROW_RETURN_NOT_OK(avg_bid_builder.Append(b.avgBid));
    ARROW_RETURN_NOT_OK(avg_bid_size_builder.Append(b.avgBidSize));
    ARROW_RETURN_NOT_OK(start_ask_builder.Append(b.startAsk));
    ARROW_RETURN_NOT_OK(end_ask_builder.Append(b.endAsk));
    ARROW_RETURN_NOT_OK(high_ask_builder.Append(b.highAsk));
    ARROW_RETURN_NOT_OK(low_ask_builder.Append(b.lowAsk));
    ARROW_RETURN_NOT_OK(avg_ask_builder.Append(b.avgAsk));
    ARROW_RETURN_NOT_OK(avg_ask_size_builder.Append(b.avgAskSize));
    ARROW_RETURN_NOT_OK(trade_quantity_builder.Append(b.tradeQuantity));
    ARROW_RETURN_NOT_OK(bid_quote_count_builder.Append(b.bid_quote_count));
    ARROW_RETURN_NOT_OK(ask_quote_count_builder.Append(b.ask_quote_count));
  }

  std::shared_ptr<arrow::Array> time_col, start_bid, end_bid, high_bid, low_bid,
    avg_bid, avg_bid_size, start_ask, end_ask, high_ask, low_ask, avg_ask,
    avg_ask_size, trade_quantity, bid_quote_count, ask_quote_count;

  ARROW_RETURN_NOT_OK(time_builder.Finish(&time_col));
  ARROW_RETURN_NOT_OK(start_bid_builder.Finish(&start_bid));
  ARROW_RETURN_NOT_OK(end_bid_builder.Finish(&end_bid));
  ARROW_RETURN_NOT_OK(high_bid_builder.Finish(&high_bid));
  ARROW_RETURN_NOT_OK(low_bid_builder.Finish(&low_bid));
  ARROW_RETURN_NOT_OK(avg_bid_builder.Finish(&avg_bid));
  ARROW_RETURN_NOT_OK(avg_bid_size_builder.Finish(&avg_bid_size));
  ARROW_RETURN_NOT_OK(start_ask_builder.Finish(&start_ask));
  ARROW_RETURN_NOT_OK(end_ask_builder.Finish(&end_ask));
  ARROW_RETURN_NOT_OK(high_ask_builder.Finish(&high_ask));
  ARROW_RETURN_NOT_OK(low_ask_builder.Finish(&low_ask));
  ARROW_RETURN_NOT_OK(avg_ask_builder.Finish(&avg_ask));
  ARROW_RETURN_NOT_OK(avg_ask_size_builder.Finish(&avg_ask_size));
  ARROW_RETURN_NOT_OK(trade_quantity_builder.Finish(&trade_quantity));
  ARROW_RETURN_NOT_OK(bid_quote_count_builder.Finish(&bid_quote_count));
  ARROW_RETURN_NOT_OK(ask_quote_count_builder.Finish(&ask_quote_count));

  auto schema = arrow::schema({
    arrow::field("time", arrow::timestamp(arrow::TimeUnit::NANO)),
    arrow::field("startBid", arrow::float64()),
    arrow::field("endBid", arrow::float64()),
    arrow::field("highBid", arrow::float64()),
    arrow::field("lowBid", arrow::float64()),
    arrow::field("avgBid", arrow::float64()),
    arrow::field("avgBidSize", arrow::float64()),
    arrow::field("startAsk", arrow::float64()),
    arrow::field("endAsk", arrow::float64()),
    arrow::field("highAsk", arrow::float64()),
    arrow::field("lowAsk", arrow::float64()),
    arrow::field("avgAsk", arrow::float64()),
    arrow::field("avgAskSize", arrow::float64()),
    arrow::field("tradeQuantity", arrow::int32()),
    arrow::field("bid_quote_count", arrow::int32()),
    arrow::field("ask_quote_count", arrow::int32()),
  });

  return arrow::RecordBatch::Make(
    schema,
    bars.size(),
    {
      time_col,
      start_bid,
      end_bid,
      high_bid,
      low_bid,
      avg_bid,
      avg_bid_size,
      start_ask,
      end_ask,
      high_ask,
      low_ask,
      avg_ask,
      avg_ask_size,
      trade_quantity,
      bid_quote_count,
      ask_quote_count,
    });
}

void saveToParquet(const std::shared_ptr<arrow::RecordBatch>& batch)
{
  // Convert RecordBatch → Table
  auto table_result = arrow::Table::FromRecordBatches({batch});
  PARQUET_THROW_NOT_OK(table_result.status());
  std::shared_ptr<arrow::Table> table = table_result.ValueOrDie();

  // Open output file
  std::shared_ptr<arrow::io::FileOutputStream> outfile;
  PARQUET_ASSIGN_OR_THROW(
    outfile,
    arrow::io::FileOutputStream::Open("src/barGeneration/bars.parquet"));

  // Writer properties
  parquet::WriterProperties::Builder builder;
  builder.compression(parquet::Compression::SNAPPY);

  // Write table
  PARQUET_THROW_NOT_OK(
    parquet::arrow::WriteTable(
      *table,
      arrow::default_memory_pool(),
      outfile,
      5'000'000, // ~5M rows ≈ 256MB for 48-byte rows
      builder.build()));
}

void saveBarsToParquet(const std::vector<bar>& bars)
{
  auto batch_result = convertToArrow(bars);
  PARQUET_THROW_NOT_OK(batch_result.status());
  auto batch = batch_result.ValueOrDie();
  saveToParquet(batch);
}