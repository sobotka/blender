/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Copyright 2011, Blender Foundation.
 */

#pragma once

#ifdef WITH_CXX_GUARDEDALLOC
#  include "MEM_guardedalloc.h"
#endif

#include "BLI_rect.h"
#include "BLI_vector.hh"

#include "COM_CompositorContext.h"
#include "COM_Device.h"
#include "COM_MemoryProxy.h"
#include "COM_Node.h"
#include "COM_NodeOperation.h"
#include <vector>

class ExecutionSystem;
class MemoryProxy;
class ReadBufferOperation;
class Device;

/**
 * \brief the execution state of a chunk in an ExecutionGroup
 * \ingroup Execution
 */
enum class eChunkExecutionState {
  /**
   * \brief chunk is not yet scheduled
   */
  NOT_SCHEDULED = 0,
  /**
   * \brief chunk is scheduled, but not yet executed
   */
  SCHEDULED = 1,
  /**
   * \brief chunk is executed.
   */
  EXECUTED = 2,
};

/**
 * \brief Class ExecutionGroup is a group of Operations that are executed as one.
 * This grouping is used to combine Operations that can be executed as one whole when
 * multi-processing.
 * \ingroup Execution
 */
class ExecutionGroup {
 private:
  // fields

  /**
   * \brief list of operations in this ExecutionGroup
   */
  blender::Vector<NodeOperation *> m_operations;

  /**
   * \brief is this ExecutionGroup an input ExecutionGroup
   * an input execution group is a group that is at the end of the calculation
   * (the output is important for the user).
   */
  bool m_is_output;

  /**
   * \brief Width of the output
   */
  unsigned int m_width;

  /**
   * \brief Height of the output
   */
  unsigned int m_height;

  /**
   * \brief size of a single chunk, being Width or of height
   * a chunk is always a square, except at the edges of the MemoryBuffer
   */
  unsigned int m_chunkSize;

  /**
   * \brief number of chunks in the x-axis
   */
  unsigned int m_x_chunks_len;

  /**
   * \brief number of chunks in the y-axis
   */
  unsigned int m_y_chunks_len;

  /**
   * \brief total number of chunks
   */
  unsigned int m_chunks_len;

  /**
   * \brief contains this ExecutionGroup a complex NodeOperation.
   */
  bool m_complex;

  /**
   * \brief can this ExecutionGroup be scheduled on an OpenCLDevice
   */
  bool m_openCL;

  /**
   * \brief Is this Execution group SingleThreaded
   */
  bool m_singleThreaded;

  /**
   * \brief what is the maximum number field of all ReadBufferOperation in this ExecutionGroup.
   * \note this is used to construct the MemoryBuffers that will be passed during execution.
   */
  unsigned int m_max_read_buffer_offset;

  /**
   * \brief All read operations of this execution group.
   */
  blender::Vector<ReadBufferOperation *> m_read_operations;

  /**
   * \brief reference to the original bNodeTree,
   * this field is only set for the 'top' execution group.
   * \note can only be used to call the callbacks for progress, status and break.
   */
  const bNodeTree *m_bTree;

  /**
   * \brief total number of chunks that have been calculated for this ExecutionGroup
   */
  unsigned int m_chunks_finished;

  /**
   * \brief m_chunk_execution_states holds per chunk the execution state. this state can be
   *   - eChunkExecutionState::NOT_SCHEDULED: not scheduled
   *   - eChunkExecutionState::SCHEDULED: scheduled
   *   - eChunkExecutionState::EXECUTED: executed
   */
  blender::Vector<eChunkExecutionState> m_chunk_execution_states;

  /**
   * \brief indicator when this ExecutionGroup has valid Operations in its vector for Execution
   * \note When building the ExecutionGroup Operations are added via recursion.
   * First a WriteBufferOperations is added, then the.
   * \note Operation containing the settings that is important for the ExecutiongGroup is added,
   * \note When this occurs, these settings are copied over from the node to the ExecutionGroup
   * \note and the Initialized flag is set to true.
   * \see complex
   * \see openCL
   */
  bool m_initialized;

  /**
   * \brief denotes boundary for border compositing
   * \note measured in pixel space
   */
  rcti m_viewerBorder;

  /**
   * \brief start time of execution
   */
  double m_executionStartTime;

  // methods
  /**
   * \brief check whether parameter operation can be added to the execution group
   * \param operation: the operation to be added
   */
  bool canContainOperation(NodeOperation *operation);

  /**
   * \brief calculate the actual chunk size of this execution group.
   * \note A chunk size is an unsigned int that is both the height and width of a chunk.
   * \note The chunk size will not be stored in the chunkSize field. This needs to be done
   * \note by the calling method.
   */
  unsigned int determineChunkSize();

  /**
   * \brief Determine the rect (minx, maxx, miny, maxy) of a chunk at a position.
   * \note Only gives useful results after the determination of the chunksize
   * \see determineChunkSize()
   */
  void determineChunkRect(rcti *rect, const unsigned int xChunk, const unsigned int yChunk) const;

  /**
   * \brief determine the number of chunks, based on the chunkSize, width and height.
   * \note The result are stored in the fields numberOfChunks, numberOfXChunks, numberOfYChunks
   */
  void determineNumberOfChunks();

  /**
   * \brief try to schedule a specific chunk.
   * \note scheduling succeeds when all input requirements are met and the chunks hasn't been
   * scheduled yet.
   * \param graph:
   * \param xChunk:
   * \param yChunk:
   * \return [true:false]
   * true: package(s) are scheduled
   * false: scheduling is deferred (depending workpackages are scheduled)
   */
  bool scheduleChunkWhenPossible(ExecutionSystem *graph, int xChunk, int yChunk);

  /**
   * \brief try to schedule a specific area.
   * \note Check if a certain area is available, when not available this are will be checked.
   * \note This method is called from other ExecutionGroup's.
   * \param graph:
   * \param area:
   * \return [true:false]
   * true: package(s) are scheduled
   * false: scheduling is deferred (depending workpackages are scheduled)
   */
  bool scheduleAreaWhenPossible(ExecutionSystem *graph, rcti *area);

  /**
   * \brief add a chunk to the WorkScheduler.
   * \param chunknumber:
   */
  bool scheduleChunk(unsigned int chunkNumber);

  /**
   * \brief determine the area of interest of a certain input area
   * \note This method only evaluates a single ReadBufferOperation
   * \param input: the input area
   * \param readOperation: The ReadBufferOperation where the area needs to be evaluated
   * \param output: the area needed of the ReadBufferOperation. Result
   */
  void determineDependingAreaOfInterest(rcti *input,
                                        ReadBufferOperation *readOperation,
                                        rcti *output);

 public:
  // constructors
  ExecutionGroup();

  // methods
  /**
   * \brief add an operation to this ExecutionGroup
   * \note this method will add input of the operations recursively
   * \note this method can create multiple ExecutionGroup's
   * \param system:
   * \param operation:
   * \return True if the operation was successfully added
   */
  bool addOperation(NodeOperation *operation);

  /**
   * \brief is this ExecutionGroup an output ExecutionGroup
   * \note An OutputExecution group are groups containing a
   * \note ViewerOperation, CompositeOperation, PreviewOperation.
   * \see NodeOperation.isOutputOperation
   */
  bool isOutputExecutionGroup() const
  {
    return this->m_is_output;
  }

  /**
   * \brief set whether this ExecutionGroup is an output
   * \param isOutput:
   */
  void setOutputExecutionGroup(bool is_output)
  {
    this->m_is_output = is_output;
  }

  /**
   * \brief determine the resolution of this ExecutionGroup
   * \param resolution:
   */
  void determineResolution(unsigned int resolution[2]);

  /**
   * \brief set the resolution of this executiongroup
   * \param resolution:
   */
  void setResolution(unsigned int resolution[2])
  {
    this->m_width = resolution[0];
    this->m_height = resolution[1];
  }

  /**
   * \brief get the width of this execution group
   */
  unsigned int getWidth() const
  {
    return m_width;
  }

  /**
   * \brief get the height of this execution group
   */
  unsigned int getHeight() const
  {
    return m_height;
  }

  /**
   * \brief does this ExecutionGroup contains a complex NodeOperation
   */
  bool isComplex() const
  {
    return m_complex;
  }

  /**
   * \brief get the output operation of this ExecutionGroup
   * \return NodeOperation *output operation
   */
  NodeOperation *getOutputOperation() const;

  /**
   * \brief compose multiple chunks into a single chunk
   * \return Memorybuffer *consolidated chunk
   */
  MemoryBuffer *constructConsolidatedMemoryBuffer(MemoryProxy *memoryProxy, rcti *rect);

  /**
   * \brief initExecution is called just before the execution of the whole graph will be done.
   * \note The implementation will calculate the chunkSize of this execution group.
   */
  void initExecution();

  /**
   * \brief get all inputbuffers needed to calculate an chunk
   * \note all inputbuffers must be executed
   * \param chunkNumber: the chunk to be calculated
   * \return (MemoryBuffer **) the inputbuffers
   */
  MemoryBuffer **getInputBuffersCPU();

  /**
   * \brief get all inputbuffers needed to calculate an chunk
   * \note all inputbuffers must be executed
   * \param chunkNumber: the chunk to be calculated
   * \return (MemoryBuffer **) the inputbuffers
   */
  MemoryBuffer **getInputBuffersOpenCL(int chunkNumber);

  /**
   * \brief allocate the outputbuffer of a chunk
   * \param chunkNumber: the number of the chunk in the ExecutionGroup
   * \param rect: the rect of that chunk
   * \see determineChunkRect
   */
  MemoryBuffer *allocateOutputBuffer(int chunkNumber, rcti *rect);

  /**
   * \brief after a chunk is executed the needed resources can be freed or unlocked.
   * \param chunknumber:
   * \param memorybuffers:
   */
  void finalizeChunkExecution(int chunkNumber, MemoryBuffer **memoryBuffers);

  /**
   * \brief deinitExecution is called just after execution the whole graph.
   * \note It will release all needed resources
   */
  void deinitExecution();

  /**
   * \brief schedule an ExecutionGroup
   * \note this method will return when all chunks have been calculated, or the execution has
   * breaked (by user)
   *
   * first the order of the chunks will be determined. This is determined by finding the
   * ViewerOperation and get the relevant information from it.
   *   - ChunkOrdering
   *   - CenterX
   *   - CenterY
   *
   * After determining the order of the chunks the chunks will be scheduled
   *
   * \see ViewerOperation
   * \param graph:
   */
  void execute(ExecutionSystem *graph);

  /**
   * \brief this method determines the MemoryProxy's where this execution group depends on.
   * \note After this method determineDependingAreaOfInterest can be called to determine
   * \note the area of the MemoryProxy.creator that has to be executed.
   * \param memoryProxies: result
   */
  void determineDependingMemoryProxies(std::vector<MemoryProxy *> *memoryProxies);

  /**
   * \brief Determine the rect (minx, maxx, miny, maxy) of a chunk.
   * \note Only gives useful results after the determination of the chunksize
   * \see determineChunkSize()
   */
  void determineChunkRect(rcti *rect, const unsigned int chunkNumber) const;

  /**
   * \brief can this ExecutionGroup be scheduled on an OpenCLDevice
   * \see WorkScheduler.schedule
   */
  bool isOpenCL();

  void setChunksize(int chunksize)
  {
    this->m_chunkSize = chunksize;
  }

  /**
   * \brief get the Render priority of this ExecutionGroup
   * \see ExecutionSystem.execute
   */
  CompositorPriority getRenderPriotrity();

  /**
   * \brief set border for viewer operation
   * \note all the coordinates are assumed to be in normalized space
   */
  void setViewerBorder(float xmin, float xmax, float ymin, float ymax);

  void setRenderBorder(float xmin, float xmax, float ymin, float ymax);

  /* allow the DebugInfo class to look at internals */
  friend class DebugInfo;

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:ExecutionGroup")
#endif
};
