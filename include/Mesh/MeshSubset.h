#pragma once

#include <set>
#include <iostream>

namespace GemCraft {

    class MeshSubset
    {
    public:
        std::set<size_t> m_Vertices;
        std::set<size_t> m_Edges;
        std::set<size_t> m_Faces;
        std::set<size_t> m_Halfedges;

        /* Initialize an empty MeshSubset. */
        MeshSubset() = default;

        /* Initialize a MeshSubset with the given vertices, edges, and faces. */
        MeshSubset(const std::set<size_t>& V, const std::set<size_t>& E, const std::set<size_t>& F, const std::set<size_t>& HE)
        {
            m_Vertices = V;
            m_Edges = E;
            m_Faces = F;
            m_Halfedges = HE;
        }

        /* Make a deep copy of the input MeshSubset and return it as a new
         * MeshSubset.
         */
        MeshSubset DeepCopy() const
        {
            std::set<size_t> newVertices = m_Vertices;
            std::set<size_t> newEdges = m_Edges;
            std::set<size_t> newFaces = m_Faces;
            std::set<size_t> newHalfedges = m_Halfedges;
            return MeshSubset(newVertices, newEdges, newFaces, newHalfedges);
        }

        /* Add a vertex to this subset. */
        void AddVertex(size_t index)
        {
            m_Vertices.insert(index);
        }

        /* Add a set of vertices to this subset. */
        void AddVertices(const std::set<size_t>& V)
        {
            for (std::set<size_t>::iterator it = V.begin(); it != V.end(); ++it) {
                m_Vertices.insert(*it);
            }
        }

        /* Delete a vertex from this subset. */
        void DeleteVertex(size_t index)
        {
            m_Vertices.erase(index);
        }

        /* Delete a set of vertices from this subset. */
        void DeleteVertices(const std::set<size_t>& V)
        {
            for (std::set<size_t>::iterator it = V.begin(); it != V.end(); ++it) {
                m_Vertices.erase(*it);
            }
        }

        /* Add an edge to this subset. */
        void AddEdge(size_t index)
        {
            m_Edges.insert(index);
        }

        /* Add a set of edges to this subset. */
        void AddEdges(const std::set<size_t>& E)
        {
            for (std::set<size_t>::iterator it = E.begin(); it != E.end(); ++it) {
                m_Edges.insert(*it);
            }
        }

        /* Delete an edge from this subset. */
        void DeleteEdge(size_t index)
        {
            m_Edges.erase(index);
        }

        /* Delete a set of edges from this subset. */
        void DeleteEdges(const std::set<size_t>& E)
        {
            for (std::set<size_t>::iterator it = E.begin(); it != E.end(); ++it) {
                m_Edges.erase(*it);
            }
        }

        /* Add a face to this subset. */
        void AddFace(size_t index)
        {
            m_Faces.insert(index);
        }

        /* Add a set of faces to this subset. */
        void AddFaces(const std::set<size_t>& F)
        {
            for (std::set<size_t>::iterator it = F.begin(); it != F.end(); ++it) {
                m_Faces.insert(*it);
            }
        }

        /* Delete a face from this subset. */
        void DeleteFace(size_t index)
        {
            m_Faces.erase(index);
        }

        /* Delete a set of faces from this subset. */
        void DeleteFaces(const std::set<size_t>& F)
        {
            for (std::set<size_t>::iterator it = F.begin(); it != F.end(); ++it) {
                m_Faces.erase(*it);
            }
        }

        /* Add an halfedge to this subset. */
        void AddHalfedge(size_t index) {
            m_Halfedges.insert(index);
        }

        /* Add a set of edges to this subset. */
        void AddHalfedges(const std::set<size_t>& HE)
        {
            for (std::set<size_t>::iterator it = HE.begin(); it != HE.end(); ++it) {
                m_Halfedges.insert(*it);
            }
        }

        /* Delete an edge from this subset. */
        void DeleteHalfedge(size_t index)
        {
            m_Halfedges.erase(index);
        }

        /* Delete a set of edges from this subset. */
        void DeleteHalfedges(const std::set<size_t>& HE)
        {
            for (std::set<size_t>::iterator it = HE.begin(); it != HE.end(); ++it) {
                m_Halfedges.erase(*it);
            }
        }

        /* Returns true if subsets are equivalent. */
        bool Equals(const MeshSubset& other) const
        {
            // == compares elements at each position; but std::set always orders elements upon insertion/initialization
            return (m_Vertices == other.m_Vertices) && (m_Edges == other.m_Edges) && (m_Faces == other.m_Faces) && (m_Halfedges == other.m_Halfedges);
        }

        /* Adds a subset's vertices, edges, faces and halfedges to this subset. */
        void addSubset(const MeshSubset& other)
        {
            this->AddVertices(other.m_Vertices);
            this->AddEdges(other.m_Edges);
            this->AddFaces(other.m_Faces);
            this->AddHalfedges(other.m_Halfedges);
        }

        /* Removes a subset's vertices, edges, faces and halfedges from this subset. */
        void deleteSubset(const MeshSubset& other)
        {
            this->DeleteVertices(other.m_Vertices);
            this->DeleteEdges(other.m_Edges);
            this->DeleteFaces(other.m_Faces);
            this->DeleteHalfedges(other.m_Halfedges);
        }

        /* Print vertices. */
        void PrintVertices()
        {
            std::cout << "Vertices: ";
            for (std::set<size_t>::iterator it = m_Vertices.begin(); it != m_Vertices.end(); ++it) {
                std::cout << *it << ", ";
            }
            std::cerr << std::endl;
        }

        /* Print edges. */
        void PrintEdges()
        {
            std::cout << "Edges: ";
            for (std::set<size_t>::iterator it = m_Edges.begin(); it != m_Edges.end(); ++it) {
                std::cout << *it << ", ";
            }
            std::cerr << std::endl;
        }

        /* Print Faces. */
        void PrintFaces()
        {
            std::cout << "Faces: ";
            for (std::set<size_t>::iterator it = m_Faces.begin(); it != m_Faces.end(); ++it) {
                std::cout << *it << ", ";
            }
            std::cerr << std::endl;
        }

        /* Print Halfedges. */
        void PrintHalfedges()
        {
            std::cout << "Halfedges: ";
            for (std::set<size_t>::iterator it = m_Halfedges.begin(); it != m_Halfedges.end(); ++it) {
                std::cout << *it << ", ";
            }
            std::cerr << std::endl;
        }
    };

} // namespace GemCraft